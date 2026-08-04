#include "terminal_ui_kit/diff/unified_diff_parser.h"

#include <charconv>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "terminal_ui_kit/core/styled_text.h"

namespace terminal_ui_kit {
namespace diff {
namespace {

bool starts_with(std::string_view line, std::string_view prefix) {
  return line.size() >= prefix.size() && line.substr(0, prefix.size()) == prefix;
}

std::string_view trim_view(std::string_view s) {
  while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.remove_prefix(1);
  while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r'))
    s.remove_suffix(1);
  return s;
}

// The path side of a "--- path" / "+++ path" / "diff --git a/path b/path"
// header. Some tools append a '\t<timestamp>' suffix; a trailing tab is used
// as the boundary so the timestamp is discarded.
std::string parse_side_path(std::string_view rest) {
  const std::size_t tab = rest.find('\t');
  const std::string_view path = tab == std::string_view::npos ? rest : rest.substr(0, tab);
  return std::string(trim_view(path));
}

bool parse_signed_int(std::string_view token, int& out) {
  if (token.empty()) return false;
  const auto parsed = std::from_chars(token.data(), token.data() + token.size(), out);
  return parsed.ec == std::errc() && parsed.ptr == token.data() + token.size();
}

// Parses a hunk range token "N" or "N,M" into a 1-based start and a count.
// A missing ",M" implies a count of 1 (the unified-diff rule).
bool parse_range_token(std::string_view token, int& start, int& count) {
  const std::size_t comma = token.find(',');
  const std::string_view start_str =
      comma == std::string_view::npos ? token : token.substr(0, comma);
  const std::string_view count_str =
      comma == std::string_view::npos ? std::string_view{} : token.substr(comma + 1);
  if (!parse_signed_int(start_str, start)) return false;
  if (count_str.empty()) {
    count = 1;
    return true;
  }
  return parse_signed_int(count_str, count);
}

// Extracts one path token from the front of `s` and returns the remainder.
// Git quotes paths that contain spaces or special characters, so a token is
// either an unquoted run delimited by the next space, or a double-quoted run
// that may itself contain spaces. (Git's C-style octal escapes are left
// as-is; this keeps the parser simple for a presentation-layer tool.)
std::string_view take_git_path(std::string_view s, std::string& out) {
  s = trim_view(s);
  if (s.empty()) return s;
  if (s.front() == '"') {
    const std::size_t close = s.find('"', 1);
    if (close == std::string_view::npos) {
      out = std::string(s.substr(1));
      return s.substr(s.size());
    }
    out = std::string(s.substr(1, close - 1));
    return s.substr(close + 1);
  }
  const std::size_t sp = s.find(' ');
  out = std::string(s.substr(0, sp));
  return sp == std::string_view::npos ? s.substr(s.size()) : s.substr(sp);
}

// Parses a hunk header "@@ -l[,c] +l[,c] @@ [section]".
bool parse_hunk_header(std::string_view header, DiffHunk& hunk) {
  std::string_view s = trim_view(header);
  if (!starts_with(s, "@@ ")) return false;
  // Retain the raw header text, including any trailing function section
  // (e.g. "@@ -1,3 +1,3 @@ int main()") that consumers may want to display.
  hunk.header = std::string(s);
  s.remove_prefix(3);
  s = trim_view(s);
  if (s.empty() || s.front() != '-') return false;
  s.remove_prefix(1);

  const std::size_t old_end = s.find(' ');
  const std::string_view old_token = s.substr(0, old_end);
  if (!parse_range_token(old_token, hunk.old_start, hunk.old_count)) return false;

  s = trim_view(s.substr(old_end == std::string_view::npos ? 0 : old_end));
  if (s.empty() || s.front() != '+') return false;
  s.remove_prefix(1);

  const std::size_t new_end = s.find(' ');
  const std::string_view new_token = s.substr(0, new_end);
  if (!parse_range_token(new_token, hunk.new_start, hunk.new_count)) return false;
  return true;
}

StyledText plain_text(std::string_view text) {
  StyledText result;
  result.append(TextSpan{std::string(text), {}, std::nullopt});
  return result;
}

}  // namespace

DiffDocument parse_unified_diff(std::string_view text) {
  DiffDocument document;

  DiffFile* file = nullptr;
  DiffHunk* hunk = nullptr;
  bool awaiting_hunk = false;
  bool file_from_git = false;
  int old_line = 0;
  int new_line = 0;

  const auto begin_file = [&](bool from_git) {
    document.files.emplace_back();
    file = &document.files.back();
    hunk = nullptr;
    awaiting_hunk = true;
    file_from_git = from_git;
  };

  // True when a "--- path" / "+++ path" line belongs to the file we are
  // currently building (its header block is still open) rather than starting
  // a brand-new file.
  const auto reuses_current_file = [&] {
    return file != nullptr && awaiting_hunk &&
           (file_from_git || file->old_path.empty() || file->new_path.empty());
  };

  std::size_t pos = 0;
  while (pos < text.size()) {
    const std::size_t eol = text.find('\n', pos);
    const std::size_t len = (eol == std::string_view::npos) ? text.size() - pos : eol - pos;
    std::string_view line = text.substr(pos, len);
    pos = (eol == std::string_view::npos) ? text.size() : eol + 1;
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);

    if (starts_with(line, "diff --git ")) {
      begin_file(/*from_git=*/true);
      const std::string_view rest = line.substr(std::string_view("diff --git ").size());
      std::string old_path;
      std::string new_path;
      const std::string_view remainder = take_git_path(rest, old_path);
      take_git_path(remainder, new_path);
      if (!old_path.empty()) file->old_path = std::move(old_path);
      if (!new_path.empty()) file->new_path = std::move(new_path);
      continue;
    }

    if (starts_with(line, "--- ")) {
      if (!reuses_current_file()) begin_file(/*from_git=*/false);
      file->old_path = parse_side_path(line.substr(4));
      awaiting_hunk = true;
      continue;
    }

    if (starts_with(line, "+++ ")) {
      if (!reuses_current_file()) begin_file(/*from_git=*/false);
      file->new_path = parse_side_path(line.substr(4));
      awaiting_hunk = true;
      continue;
    }

    if (starts_with(line, "@@ ")) {
      if (file == nullptr) begin_file(/*from_git=*/false);
      DiffHunk parsed;
      if (parse_hunk_header(line, parsed)) {
        file->hunks.push_back(std::move(parsed));
        hunk = &file->hunks.back();
        old_line = hunk->old_start;
        new_line = hunk->new_start;
        awaiting_hunk = false;
      }
      // A malformed hunk header is skipped; the parser stays positioned to
      // pick up any subsequent well-formed hunk.
      continue;
    }

    if (starts_with(line, "Binary files ") && line.find(" differ") != std::string_view::npos) {
      if (file == nullptr) begin_file(/*from_git=*/false);
      file->is_binary = true;
      // "Binary files <old> and <new> differ"
      std::string_view rest = line.substr(std::string_view("Binary files ").size());
      const std::size_t and_pos = rest.find(" and ");
      if (and_pos != std::string_view::npos) {
        const std::string_view old_path = trim_view(rest.substr(0, and_pos));
        std::string_view new_path = rest.substr(and_pos + 5);
        // The line ends with " differ"; strip that trailing word.
        constexpr std::string_view kDiffer = " differ";
        if (new_path.size() >= kDiffer.size() &&
            new_path.substr(new_path.size() - kDiffer.size()) == kDiffer)
          new_path.remove_suffix(kDiffer.size());
        new_path = trim_view(new_path);
        // Prefer the paths parsed from "diff --git" (which handle quoting);
        // only fall back to the binary notice when they are still unset.
        if (!old_path.empty() && file->old_path.empty()) file->old_path = std::string(old_path);
        if (!new_path.empty() && file->new_path.empty()) file->new_path = std::string(new_path);
      }
      awaiting_hunk = true;
      continue;
    }

    if (starts_with(line, "GIT binary patch")) {
      if (file == nullptr) begin_file(/*from_git=*/false);
      file->is_binary = true;
      awaiting_hunk = true;
      continue;
    }

    // Content lines belong to the open hunk.
    if (hunk != nullptr && !line.empty() &&
        (line.front() == ' ' || line.front() == '+' || line.front() == '-' ||
         line.front() == '\\')) {
      DiffLine diff_line;
      const char marker = line.front();
      std::string_view content = line.substr(1);
      switch (marker) {
        case ' ':
          diff_line.type = DiffLineType::kContext;
          diff_line.old_line = old_line;
          diff_line.new_line = new_line;
          ++old_line;
          ++new_line;
          break;
        case '+':
          diff_line.type = DiffLineType::kAddition;
          diff_line.new_line = new_line;
          ++new_line;
          break;
        case '-':
          diff_line.type = DiffLineType::kDeletion;
          diff_line.old_line = old_line;
          ++old_line;
          break;
        default:
          diff_line.type = DiffLineType::kNoNewline;
          break;
      }
      diff_line.content = plain_text(content);
      hunk->lines.push_back(std::move(diff_line));
      continue;
    }

    // Any other line (metadata in the header region, or the first non-content
    // line after a hunk) ends the current hunk and is otherwise ignored.
    hunk = nullptr;
  }

  return document;
}

}  // namespace diff
}  // namespace terminal_ui_kit
