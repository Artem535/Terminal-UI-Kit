#include "terminal_ui_kit/diff/unified_diff.h"

#include <cctype>
#include <sstream>

namespace terminal_ui_kit {

namespace {
// Helper to trim leading "\r" and trailing "\n" from a line read from the diff.
std::string CleanLine(const std::string& line) {
  std::string_view view = line;
  if (!view.empty() && view.back() == '\n') view.remove_suffix(1);
  if (!view.empty() && view.back() == '\r') view.remove_suffix(1);
  return std::string(view);
}

// Parse a hunk header of the form "@@ -old_start,old_len +new_start,new_len @@"
bool ParseHunkHeader(const std::string& header, DiffHunk& out) {
  // Expected format: "@@ -old_start,old_len +new_start,new_len @@" where the lengths are optional.
  int o_start = 0, o_len = 0, n_start = 0, n_len = 0;
  // Try parsing with both lengths present.
  int matched = std::sscanf(header.c_str(), "@@ -%d,%d +%d,%d @@", &o_start, &o_len, &n_start, &n_len);
  if (matched == 4) {
    out.old_start = static_cast<std::size_t>(o_start);
    out.old_lines = static_cast<std::size_t>(o_len);
    out.new_start = static_cast<std::size_t>(n_start);
    out.new_lines = static_cast<std::size_t>(n_len);
    return true;
  }
  // Try without explicit lengths (defaults to 1).
  matched = std::sscanf(header.c_str(), "@@ -%d +%d @@", &o_start, &n_start);
  if (matched == 2) {
    out.old_start = static_cast<std::size_t>(o_start);
    out.old_lines = 1;
    out.new_start = static_cast<std::size_t>(n_start);
    out.new_lines = 1;
    return true;
  }
  // Try with old length only.
  matched = std::sscanf(header.c_str(), "@@ -%d,%d +%d @@", &o_start, &o_len, &n_start);
  if (matched == 3) {
    out.old_start = static_cast<std::size_t>(o_start);
    out.old_lines = static_cast<std::size_t>(o_len);
    out.new_start = static_cast<std::size_t>(n_start);
    out.new_lines = 1;
    return true;
  }
  // Try with new length only.
  matched = std::sscanf(header.c_str(), "@@ -%d +%d,%d @@", &o_start, &n_start, &n_len);
  if (matched == 3) {
    out.old_start = static_cast<std::size_t>(o_start);
    out.old_lines = 1;
    out.new_start = static_cast<std::size_t>(n_start);
    out.new_lines = static_cast<std::size_t>(n_len);
    return true;
  }
  return false;
}
}  // namespace

std::vector<DiffFile> UnifiedDiffParser::Parse(const std::string& diff_text) {
  std::vector<DiffFile> files;
  std::istringstream stream(diff_text);
  std::string line_raw;
  DiffFile current_file;
  DiffHunk current_hunk;
  std::size_t old_line = 0, new_line = 0;
  enum class State { Idle, Header, Hunk } state = State::Idle;

  while (std::getline(stream, line_raw)) {
    std::string line = CleanLine(line_raw);
    if (line.rfind("--- ", 0) == 0) {
      // start of a new file entry
      if (!current_file.old_path.empty() || !current_file.hunks.empty()) {
        files.push_back(std::move(current_file));
        current_file = DiffFile{};
      }
      current_file.old_path = line.substr(4);
      state = State::Header;
      continue;
    }
    if (line.rfind("+++ ", 0) == 0 && state == State::Header) {
      current_file.new_path = line.substr(4);
      continue;
    }
    // Binary diff notice
    if (line.find("Binary files ") != std::string::npos) {
      current_file.is_binary = true;
      continue;
    }
    if (line.rfind("@@ ", 0) == 0) {
      // finalize previous hunk
      if (!current_hunk.lines.empty()) {
        current_file.hunks.push_back(std::move(current_hunk));
        current_hunk = DiffHunk{};
      }
      if (!ParseHunkHeader(line, current_hunk)) {
        // malformed hunk – abort parsing this file
        continue;
      }
      old_line = current_hunk.old_start;
      new_line = current_hunk.new_start;
      state = State::Hunk;
      continue;
    }
    if (state == State::Hunk) {
      DiffLine dline;
      if (!line.empty()) {
        char marker = line[0];
        std::string content = line.substr(1);
        switch (marker) {
          case '+':
            dline.type = DiffLine::Type::Added;
            dline.new_line = new_line++;
            dline.old_line = std::nullopt;
            break;
          case '-':
            dline.type = DiffLine::Type::Removed;
            dline.old_line = old_line++;
            dline.new_line = std::nullopt;
            break;
          case ' ':
            dline.type = DiffLine::Type::Context;
            dline.old_line = old_line++;
            dline.new_line = new_line++;
            break;
          case '\\':  // No newline at end of file marker "\\ No newline at end of file"
            dline.type = DiffLine::Type::NoNewline;
            dline.text = line;  // keep full marker line
            dline.old_line = std::nullopt;
            dline.new_line = std::nullopt;
            break;
          default:
            // Unexpected line – treat as context to keep data loss minimal.
            dline.type = DiffLine::Type::Context;
            dline.text = line;
            dline.old_line = old_line++;
            dline.new_line = new_line++;
            break;
        }
        dline.text = (marker == '+' || marker == '-' || marker == ' ') ? content : line;
      } else {
        // Empty line – treat as context.
        dline.type = DiffLine::Type::Context;
        dline.text = "";
        dline.old_line = old_line++;
        dline.new_line = new_line++;
      }
      current_hunk.lines.push_back(std::move(dline));
    }
  }
  // push last pending structures
  if (!current_hunk.lines.empty()) {
    current_file.hunks.push_back(std::move(current_hunk));
  }
  if (!current_file.old_path.empty() || !current_file.hunks.empty()) {
    files.push_back(std::move(current_file));
  }
  return files;
}

}  // namespace terminal_ui_kit
