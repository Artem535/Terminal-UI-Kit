#include "terminal_ui_kit/diff/unified_diff_parser.h"

#include <cctype>
#include <climits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace terminal_ui_kit::diff {
namespace {

// Splits text into lines, stripping a trailing '\r' from each line so that
// CRLF diffs parse identically to LF diffs. A trailing newline does not
// produce an extra empty line.
std::vector<std::string_view> SplitLines(std::string_view text) {
  std::vector<std::string_view> lines;
  std::size_t start = 0;
  while (start <= text.size()) {
    std::size_t end = text.find('\n', start);
    std::string_view line =
        text.substr(start, end == std::string_view::npos ? text.size() - start : end - start);
    if (!line.empty() && line.back() == '\r') {
      line.remove_suffix(1);
    }
    lines.push_back(line);
    if (end == std::string_view::npos) {
      break;
    }
    start = end + 1;
  }
  return lines;
}

bool StartsWith(std::string_view line, std::string_view prefix) {
  return line.size() >= prefix.size() && line.substr(0, prefix.size()) == prefix;
}

// Strips a trailing timestamp (everything from the first tab) from a file
// path header, e.g. "a/file.txt\t2024-01-01 12:00:00" -> "a/file.txt".
std::string StripTimestamp(std::string_view path) {
  std::size_t tab = path.find('\t');
  if (tab != std::string_view::npos) {
    path = path.substr(0, tab);
  }
  return std::string(path);
}

// Strips the leading "a/" or "b/" prefix that git adds to `---`/`+++` paths,
// so that all stored paths use the bare form (matching `rename from`/`rename
// to` and `diff -u` output). Paths without such a prefix (e.g. "/dev/null")
// are returned unchanged.
std::string StripGitPrefix(std::string_view path) {
  if (path.size() >= 2 && path[1] == '/' && (path[0] == 'a' || path[0] == 'b')) {
    path.remove_prefix(2);
  }
  return std::string(path);
}

// Parses a non-negative integer at the start of `s`, advancing `s` past it.
// Returns std::nullopt if `s` does not begin with a digit sequence or if the
// value would overflow `int` (so adversarial malformed input is rejected
// predictably rather than invoking undefined behavior).
std::optional<int> ParseInt(std::string_view& s) {
  if (s.empty() || !std::isdigit(static_cast<unsigned char>(s[0]))) {
    return std::nullopt;
  }
  int value = 0;
  while (!s.empty() && std::isdigit(static_cast<unsigned char>(s[0]))) {
    const int digit = s[0] - '0';
    if (value > (INT_MAX - digit) / 10) {
      return std::nullopt;
    }
    value = value * 10 + digit;
    s.remove_prefix(1);
  }
  return value;
}

struct HunkHeader {
  int old_start;
  int new_start;
};

// Parses a hunk header of the form "@@ -<old>[,<count>] +<new>[,<count>] @@"
// and returns the old/new start line numbers. Returns std::nullopt for any
// malformed header so the caller can skip it predictably.
std::optional<HunkHeader> ParseHunkHeader(std::string_view header) {
  if (!StartsWith(header, "@@ ")) {
    return std::nullopt;
  }
  std::string_view rest = header.substr(3);
  if (rest.empty() || rest[0] != '-') {
    return std::nullopt;
  }
  rest.remove_prefix(1);
  auto old_start = ParseInt(rest);
  if (!old_start) {
    return std::nullopt;
  }
  if (!rest.empty() && rest[0] == ',') {
    rest.remove_prefix(1);
    if (!ParseInt(rest)) {
      return std::nullopt;
    }
  }
  if (rest.empty() || rest[0] != ' ') {
    return std::nullopt;
  }
  rest.remove_prefix(1);
  if (rest.empty() || rest[0] != '+') {
    return std::nullopt;
  }
  rest.remove_prefix(1);
  auto new_start = ParseInt(rest);
  if (!new_start) {
    return std::nullopt;
  }
  if (!rest.empty() && rest[0] == ',') {
    rest.remove_prefix(1);
    if (!ParseInt(rest)) {
      return std::nullopt;
    }
  }
  if (!StartsWith(rest, " @@")) {
    return std::nullopt;
  }
  return HunkHeader{*old_start, *new_start};
}

}  // namespace

DiffDocument parse_unified_diff(std::string_view text) {
  DiffDocument document;
  std::vector<std::string_view> lines = SplitLines(text);

  std::optional<DiffFile> current_file;
  std::optional<DiffHunk> current_hunk;
  int old_line = 0;
  int new_line = 0;

  auto finalize_hunk = [&]() {
    if (current_hunk && current_file) {
      current_file->hunks.push_back(std::move(*current_hunk));
    }
    current_hunk.reset();
  };

  auto finalize_file = [&]() {
    finalize_hunk();
    if (current_file) {
      document.files.push_back(std::move(*current_file));
    }
    current_file.reset();
  };

  auto ensure_file = [&]() {
    if (!current_file) {
      current_file = DiffFile{};
    }
  };

  auto begin_hunk = [&](std::string_view line) {
    if (auto parsed = ParseHunkHeader(line)) {
      current_hunk = DiffHunk{};
      current_hunk->header = std::string(line);
      old_line = parsed->old_start;
      new_line = parsed->new_start;
    }
  };

  for (std::string_view line : lines) {
    if (current_hunk) {
      // Inside a hunk body.
      if (line.empty()) {
        continue;
      }
      const char marker = line[0];
      if (marker == ' ') {
        DiffLine diff_line;
        diff_line.type = DiffLineType::kContext;
        diff_line.old_line = old_line++;
        diff_line.new_line = new_line++;
        diff_line.content.append(TextSpan{std::string(line.substr(1)), TextStyle{}, std::nullopt});
        current_hunk->lines.push_back(std::move(diff_line));
      } else if (marker == '+') {
        DiffLine diff_line;
        diff_line.type = DiffLineType::kAddition;
        diff_line.new_line = new_line++;
        diff_line.content.append(TextSpan{std::string(line.substr(1)), TextStyle{}, std::nullopt});
        current_hunk->lines.push_back(std::move(diff_line));
      } else if (marker == '-') {
        DiffLine diff_line;
        diff_line.type = DiffLineType::kDeletion;
        diff_line.old_line = old_line++;
        diff_line.content.append(TextSpan{std::string(line.substr(1)), TextStyle{}, std::nullopt});
        current_hunk->lines.push_back(std::move(diff_line));
      } else if (marker == '\\') {
        // "\ No newline at end of file" marker; does not add a line.
        continue;
      } else if (StartsWith(line, "@@ ")) {
        finalize_hunk();
        begin_hunk(line);
      } else if (StartsWith(line, "diff --git ")) {
        finalize_file();
        ensure_file();
      } else if (StartsWith(line, "Binary files ")) {
        if (current_file) {
          current_file->is_binary = true;
        }
        finalize_file();
      } else {
        // Unrecognized line inside a hunk: end the hunk and ignore it.
        finalize_hunk();
      }
    } else {
      // Outside a hunk.
      if (StartsWith(line, "diff --git ")) {
        finalize_file();
        ensure_file();
      } else if (StartsWith(line, "--- ")) {
        ensure_file();
        current_file->old_path = StripGitPrefix(StripTimestamp(line.substr(4)));
      } else if (StartsWith(line, "+++ ")) {
        ensure_file();
        current_file->new_path = StripGitPrefix(StripTimestamp(line.substr(4)));
      } else if (StartsWith(line, "rename from ")) {
        ensure_file();
        current_file->old_path = std::string(line.substr(12));
      } else if (StartsWith(line, "rename to ")) {
        ensure_file();
        current_file->new_path = std::string(line.substr(10));
      } else if (StartsWith(line, "copy from ")) {
        ensure_file();
        current_file->old_path = std::string(line.substr(10));
      } else if (StartsWith(line, "copy to ")) {
        ensure_file();
        current_file->new_path = std::string(line.substr(8));
      } else if (StartsWith(line, "@@ ")) {
        ensure_file();
        begin_hunk(line);
      } else if (StartsWith(line, "Binary files ")) {
        ensure_file();
        current_file->is_binary = true;
        finalize_file();
      }
      // Other metadata lines (index, new file mode, etc.) are ignored.
    }
  }

  finalize_file();
  return document;
}

}  // namespace terminal_ui_kit::diff
