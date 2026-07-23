#include "terminal_ui_kit/core/text_wrap.h"

#include <algorithm>
#include <cstdint>

namespace terminal_ui_kit {

namespace {

// Byte length of the UTF-8 codepoint starting at `lead_byte`. Invalid
// leading bytes are treated as length 1 so decoding never gets stuck.
int utf8_codepoint_length(unsigned char lead_byte) {
  if ((lead_byte & 0x80) == 0x00) return 1;
  if ((lead_byte & 0xE0) == 0xC0) return 2;
  if ((lead_byte & 0xF0) == 0xE0) return 3;
  if ((lead_byte & 0xF8) == 0xF0) return 4;
  return 1;
}

std::vector<std::string_view> split_into_codepoints(std::string_view text) {
  std::vector<std::string_view> codepoints;
  std::size_t i = 0;
  while (i < text.size()) {
    int length = utf8_codepoint_length(static_cast<unsigned char>(text[i]));
    length = std::min<int>(length, static_cast<int>(text.size() - i));
    codepoints.push_back(text.substr(i, length));
    i += length;
  }
  return codepoints;
}

bool is_ascii_space(std::string_view codepoint) {
  return codepoint.size() == 1 && (codepoint[0] == ' ' || codepoint[0] == '\t');
}

}  // namespace

std::vector<std::string> wrap_plain_text(std::string_view text, int width) {
  width = std::max(width, 1);

  std::vector<std::string_view> codepoints = split_into_codepoints(text);
  std::vector<std::string> lines;
  std::string current_line;
  int current_width = 0;

  auto flush_line = [&]() {
    lines.push_back(current_line);
    current_line.clear();
    current_width = 0;
  };

  std::size_t i = 0;
  while (i < codepoints.size()) {
    std::size_t start = i;
    bool is_space_run = is_ascii_space(codepoints[i]);
    while (i < codepoints.size() && is_ascii_space(codepoints[i]) == is_space_run) {
      ++i;
    }
    int run_width = static_cast<int>(i - start);

    if (is_space_run) {
      if (current_width == 0) continue;  // drop leading whitespace on a line
      if (current_width + run_width > width) {
        flush_line();
        continue;  // the break itself replaces this whitespace run
      }
      for (std::size_t k = start; k < i; ++k) current_line += codepoints[k];
      current_width += run_width;
      continue;
    }

    if (run_width > width) {
      // Hard-break a word that can never fit on one line, one codepoint at
      // a time so multi-byte UTF-8 sequences stay intact.
      for (std::size_t k = start; k < i; ++k) {
        if (current_width == width) flush_line();
        current_line += codepoints[k];
        ++current_width;
      }
      continue;
    }

    if (current_width > 0 && current_width + run_width > width) {
      flush_line();
    }
    for (std::size_t k = start; k < i; ++k) current_line += codepoints[k];
    current_width += run_width;
  }

  if (!current_line.empty() || lines.empty()) {
    lines.push_back(current_line);
  }

  return lines;
}

std::vector<WrappedSegment> wrap_plain_text_with_offsets(
    std::string_view text, int width) {
  width = std::max(width, 1);

  std::vector<std::string_view> codepoints = split_into_codepoints(text);
  std::vector<WrappedSegment> lines;
  std::string current_line;
  int current_width = 0;
  std::size_t current_offset = 0;

  auto flush_line = [&]() {
    lines.emplace_back(current_line, current_offset);
    current_line.clear();
    current_width = 0;
  };

  std::size_t i = 0;
  while (i < codepoints.size()) {
    std::size_t start = i;
    bool is_space_run = is_ascii_space(codepoints[i]);
    while (i < codepoints.size() && is_ascii_space(codepoints[i]) == is_space_run) {
      ++i;
    }
    int run_width = static_cast<int>(i - start);

    if (is_space_run) {
      if (current_width == 0) continue;
      if (current_width + run_width > width) {
        flush_line();
        continue;
      }
      for (std::size_t k = start; k < i; ++k) current_line += codepoints[k];
      current_width += run_width;
      continue;
    }

    if (run_width > width) {
      for (std::size_t k = start; k < i; ++k) {
        if (current_width == 0) {
          current_offset = codepoints[k].data() - text.data();
        }
        if (current_width == width) {
          flush_line();
          current_offset = codepoints[k].data() - text.data();
        }
        current_line += codepoints[k];
        ++current_width;
      }
      continue;
    }

    if (current_width > 0 && current_width + run_width > width) {
      flush_line();
    }
    if (current_width == 0) {
      current_offset = codepoints[start].data() - text.data();
    }
    for (std::size_t k = start; k < i; ++k) current_line += codepoints[k];
    current_width += run_width;
  }

  if (!current_line.empty() || lines.empty()) {
    lines.emplace_back(current_line, current_offset);
  }

  return lines;
}

}  // namespace terminal_ui_kit
