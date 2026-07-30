#include "terminal_ui_kit/diff/unified_diff_parser.h"

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace terminal_ui_kit {
namespace {

struct ParserState {
  std::string_view input;
  std::size_t current_line = 0;  // 0-based, error_line reports 1-based
  std::string line;
  bool has_error = false;
  std::size_t error_line = 0;
  std::string error_message;
};

// Returns the next line from the input, or false if no more lines.
bool next_line(ParserState& state) {
  if (state.input.empty()) return false;

  std::size_t pos = state.input.find('\n');
  if (pos == std::string_view::npos) {
    state.line = std::string(state.input);
    state.input = {};
    ++state.current_line;
    return true;
  }

  state.line = std::string(state.input.substr(0, pos));
  // Handle \r\n
  if (!state.line.empty() && state.line.back() == '\r') {
    state.line.pop_back();
  }
  state.input.remove_prefix(pos + 1);
  ++state.current_line;
  return true;
}

void record_error(ParserState& state, std::string message) {
  if (!state.has_error) {
    state.has_error = true;
    state.error_line = state.current_line;
    state.error_message = std::move(message);
  }
}

// Returns true if the line starts with "--- " (file header prefix).
bool is_old_file_header(std::string_view line) { return line.starts_with("--- "); }

// Returns true if the line starts with "+++ " (file header prefix).
bool is_new_file_header(std::string_view line) { return line.starts_with("+++ "); }

// Returns true if the line starts with "@@" (hunk header).
bool is_hunk_header(std::string_view line) { return line.starts_with("@@"); }

// Returns true if the line is a "No newline at end of file" marker.
bool is_no_newline_marker(std::string_view line) {
  return line.starts_with("\\ ") &&
         line.find("No newline at end of file") != std::string_view::npos;
}

// Returns true if the line is a binary file notice.
bool is_binary_notice(std::string_view line) {
  return line.starts_with("Binary files ") && line.find(" differ") != std::string_view::npos;
}

// Strips leading and trailing whitespace from a string_view.
std::string_view strip(std::string_view s) {
  while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.remove_prefix(1);
  while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.remove_suffix(1);
  return s;
}

// Helper: parse a non-negative integer from a string_view, requiring that
// the entire view is consumed. Returns true on success.
bool parse_int_exact(std::string_view s, int& value) {
  if (s.empty()) return false;
  const char* first = s.data();
  const char* last = s.data() + s.size();
  auto [ptr, ec] = std::from_chars(first, last, value);
  return ec == std::errc{} && ptr == last && value >= 0;
}

// Parse a hunk header of the form:
//   @@ -old_start[,old_count] +new_start[,new_count] @@ [section_heading]
//
// old_count and new_count default to 1 when omitted.
// Returns true on success.
bool parse_hunk_header(std::string_view header, int& old_start, int& old_count, int& new_start,
                       int& new_count) {
  auto pos = header.find("@@");
  if (pos == std::string_view::npos) return false;
  header.remove_prefix(pos + 2);
  header = strip(header);

  // Expect "-old_start[,old_count]"
  if (header.empty() || header.front() != '-') return false;
  header.remove_prefix(1);

  auto space = header.find(' ');
  if (space == std::string_view::npos) return false;

  auto old_range = header.substr(0, space);
  auto comma = old_range.find(',');
  if (comma != std::string_view::npos) {
    if (!parse_int_exact(old_range.substr(0, comma), old_start)) return false;
    if (!parse_int_exact(old_range.substr(comma + 1), old_count)) return false;
  } else {
    if (!parse_int_exact(old_range, old_start)) return false;
    old_count = 1;
  }

  header.remove_prefix(space + 1);
  header = strip(header);

  // Expect "+new_start[,new_count] @@"
  if (header.empty() || header.front() != '+') return false;
  header.remove_prefix(1);

  auto next_ats = header.find("@@");
  if (next_ats == std::string_view::npos) return false;

  auto new_range = strip(header.substr(0, next_ats));
  comma = new_range.find(',');
  if (comma != std::string_view::npos) {
    if (!parse_int_exact(new_range.substr(0, comma), new_start)) return false;
    if (!parse_int_exact(new_range.substr(comma + 1), new_count)) return false;
  } else {
    if (!parse_int_exact(new_range, new_start)) return false;
    new_count = 1;
  }

  if (!header.substr(next_ats).starts_with("@@")) return false;
  return true;
}

// Parse the old path, stripping optional tab+timestamp.
std::string parse_old_path(std::string_view line) {
  line.remove_prefix(4);
  auto tab = line.find('\t');
  if (tab != std::string_view::npos) {
    line = line.substr(0, tab);
  }
  return std::string(line);
}

// Parse the new path, stripping optional tab+timestamp.
std::string parse_new_path(std::string_view line) {
  line.remove_prefix(4);
  auto tab = line.find('\t');
  if (tab != std::string_view::npos) {
    line = line.substr(0, tab);
  }
  return std::string(line);
}

}  // namespace

DiffParseResult parse_unified_diff(std::string_view input) {
  DiffParseResult result;
  ParserState state{};
  state.input = input;

  DiffFile current_file;
  bool in_file = false;
  bool in_hunk = false;
  bool had_old_header = false;
  int expected_old_line = 0;
  int expected_new_line = 0;

  while (next_line(state)) {
    const auto& line = state.line;

    // Skip "No newline at end of file" markers.
    if (is_no_newline_marker(line)) {
      continue;
    }

    // Binary file notice.
    if (is_binary_notice(line)) {
      if (in_hunk) {
        in_hunk = false;
      }
      current_file.is_binary = true;
      continue;
    }

    // --- Old file header
    if (is_old_file_header(line)) {
      if (in_hunk) {
        in_hunk = false;
      }
      if (in_file) {
        result.files.push_back(std::move(current_file));
        current_file = DiffFile{};
        in_hunk = false;
      }
      current_file.old_path = parse_old_path(line);
      had_old_header = true;
      in_file = true;
      continue;
    }

    // +++ New file header
    if (is_new_file_header(line)) {
      if (in_hunk) {
        in_hunk = false;
      }
      if (!had_old_header) {
        if (in_file) {
          result.files.push_back(std::move(current_file));
          current_file = DiffFile{};
          in_hunk = false;
        }
        current_file.new_path = parse_new_path(line);
        in_file = true;
      } else {
        current_file.new_path = parse_new_path(line);
        had_old_header = false;
      }
      continue;
    }

    // Hunk header
    if (is_hunk_header(line)) {
      in_hunk = false;

      DiffHunk hunk;
      hunk.header = line;
      if (!parse_hunk_header(line, hunk.old_start, hunk.old_count, hunk.new_start,
                             hunk.new_count)) {
        record_error(state, "malformed hunk header");
        hunk.old_start = 0;
        hunk.old_count = 0;
        hunk.new_start = 0;
        hunk.new_count = 0;
      }
      current_file.hunks.push_back(std::move(hunk));
      in_hunk = true;
      expected_old_line = current_file.hunks.back().old_start;
      expected_new_line = current_file.hunks.back().new_start;
      continue;
    }

    // Line within a hunk
    if (in_hunk && !current_file.hunks.empty()) {
      DiffLine diff_line;

      if (!line.empty() && line[0] == ' ') {
        diff_line.type = DiffLineType::kContext;
        diff_line.old_line = expected_old_line;
        diff_line.new_line = expected_new_line;
        diff_line.content = line.substr(1);
        current_file.hunks.back().lines.push_back(std::move(diff_line));
        ++expected_old_line;
        ++expected_new_line;
      } else if (!line.empty() && line[0] == '+') {
        diff_line.type = DiffLineType::kAddition;
        diff_line.new_line = expected_new_line;
        diff_line.content = line.substr(1);
        current_file.hunks.back().lines.push_back(std::move(diff_line));
        ++expected_new_line;
      } else if (!line.empty() && line[0] == '-') {
        diff_line.type = DiffLineType::kDeletion;
        diff_line.old_line = expected_old_line;
        diff_line.content = line.substr(1);
        current_file.hunks.back().lines.push_back(std::move(diff_line));
        ++expected_old_line;
      } else if (line.empty()) {
        diff_line.type = DiffLineType::kContext;
        diff_line.old_line = expected_old_line;
        diff_line.new_line = expected_new_line;
        diff_line.content = line;
        current_file.hunks.back().lines.push_back(std::move(diff_line));
        ++expected_old_line;
        ++expected_new_line;
      } else {
        record_error(state, "unexpected line content in hunk");
        diff_line.type = DiffLineType::kContext;
        diff_line.old_line = expected_old_line;
        diff_line.new_line = expected_new_line;
        diff_line.content = line;
        current_file.hunks.back().lines.push_back(std::move(diff_line));
        ++expected_old_line;
        ++expected_new_line;
      }
      continue;
    }

    // Unrecognised lines are skipped silently.
  }

  if (in_file) {
    result.files.push_back(std::move(current_file));
  }

  if (state.has_error) {
    result.error_line = state.error_line;
    result.error_message = std::move(state.error_message);
  }

  return result;
}

}  // namespace terminal_ui_kit
