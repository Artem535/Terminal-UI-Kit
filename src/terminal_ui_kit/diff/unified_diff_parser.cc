#include "terminal_ui_kit/diff/unified_diff_parser.h"

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

// Builds a single-span StyledText from a plain line of diff content.
StyledText make_styled(std::string_view text) {
  StyledText result;
  result.append(TextSpan{std::string(text), TextStyle{}, std::nullopt});
  return result;
}

// Splits a git "C-quoted" path token (the form git emits for paths with
// special characters) back into the plain path string. A token that does not
// start with a double quote is returned unchanged.
std::string unquote_git_path(std::string_view raw) {
  if (raw.empty() || raw.front() != '"') return std::string(raw);
  std::string out;
  for (std::size_t i = 1; i + 1 < raw.size(); ++i) {
    const char c = raw[i];
    if (c != '\\') {
      out.push_back(c);
      continue;
    }
    if (i + 1 >= raw.size()) break;
    const char next = raw[++i];
    switch (next) {
      case 'n':
        out.push_back('\n');
        break;
      case 't':
        out.push_back('\t');
        break;
      case '"':
        out.push_back('"');
        break;
      case '\\':
        out.push_back('\\');
        break;
      default:
        if (next >= '0' && next <= '7') {
          int value = next - '0';
          int digits = 1;
          while (digits < 3 && i + 1 < raw.size() && raw[i + 1] >= '0' && raw[i + 1] <= '7') {
            value = value * 8 + (raw[++i] - '0');
            ++digits;
          }
          out.push_back(static_cast<char>(value));
        } else {
          out.push_back(next);
        }
        break;
    }
  }
  return out;
}

// Reads the next whitespace-delimited word starting at *pos, advancing *pos
// past the word. A word beginning with `"` is read to its closing quote so
// that spaces inside a quoted path do not split it. Returns {} at end of
// input.
std::string_view next_word(std::string_view s, std::size_t& pos) {
  while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t')) ++pos;
  if (pos >= s.size()) return {};
  const std::size_t start = pos;
  if (s[pos] == '"') {
    ++pos;
    while (pos < s.size()) {
      if (s[pos] == '\\' && pos + 1 < s.size()) {
        pos += 2;
        continue;
      }
      if (s[pos] == '"') {
        ++pos;
        break;
      }
      ++pos;
    }
    return s.substr(start, pos - start);
  }
  while (pos < s.size() && s[pos] != ' ' && s[pos] != '\t') ++pos;
  return s.substr(start, pos - start);
}

// Normalizes one path token from a `diff --git a/old b/new` header: removes
// the trailing quote-aware prefix (`a/` for the old side, `b/` for the new
// side) and any git C-quoting.
std::string path_from_token(std::string_view token, char prefix) {
  std::string path = unquote_git_path(token);
  std::string_view view = path;
  if (view.size() >= 2 && view[1] == '/' &&
      (view[0] == prefix || view[0] == 'a' || view[0] == 'b')) {
    view.remove_prefix(2);
  }
  return std::string(view);
}

// Parses a `--- a/path` / `+++ b/path` header line into a bare path,
// preserving `/dev/null` and stripping the `a/` / `b/` prefix.
std::string dashed_path(std::string_view line, std::string_view marker) {
  const std::string_view rest = line.substr(marker.size());
  const std::string path = unquote_git_path(rest);
  std::string_view view = path;
  if (view == "/dev/null") return "/dev/null";
  if (view.size() >= 2 && view[1] == '/' && (view[0] == 'a' || view[0] == 'b')) {
    view.remove_prefix(2);
  }
  return std::string(view);
}

// Parses one old/new range token such as `-1,5`, `+5`, `,5`, or `-0,0` into
// a 1-based start (0 when the side has no lines) and a line count (1 when the
// count is omitted, as git does for single-line hunks). Returns false on
// malformed input.
bool parse_range_token(std::string_view token, int& start, int& count) {
  std::size_t i = 0;
  if (i < token.size() && (token[i] == '+' || token[i] == '-')) ++i;
  const std::size_t number_begin = i;
  while (i < token.size() && token[i] >= '0' && token[i] <= '9') ++i;
  const bool has_start = i > number_begin;
  start = 0;
  count = 1;
  if (has_start) {
    int value = 0;
    for (std::size_t k = number_begin; k < i; ++k) {
      value = value * 10 + (token[k] - '0');
    }
    start = value;
  }
  if (i < token.size()) {
    if (token[i] != ',') return false;
    ++i;
    const std::size_t length_begin = i;
    while (i < token.size() && token[i] >= '0' && token[i] <= '9') ++i;
    if (i > length_begin) {
      int value = 0;
      for (std::size_t k = length_begin; k < i; ++k) {
        value = value * 10 + (token[k] - '0');
      }
      count = value;
    }
    if (i != token.size()) return false;
  } else if (!has_start) {
    return false;
  }
  return true;
}

// Parses `@@ -old_start,old_len +new_start,new_len @@ [section]`. Returns
// false when the ranges cannot be extracted; in that case the caller still
// records the raw header but treats the hunk as having no declared lines.
bool parse_hunk_header(std::string_view line, int& old_start, int& old_len, int& new_start,
                       int& new_len) {
  const std::size_t sp1 = line.find(' ', 2);
  if (sp1 == std::string_view::npos) return false;
  const std::size_t sp2 = line.find(' ', sp1 + 1);
  if (sp2 == std::string_view::npos) return false;
  std::string_view old_raw = line.substr(sp1 + 1, sp2 - sp1 - 1);
  const std::size_t trailing = line.find("@@", sp2 + 1);
  std::string_view new_raw = (trailing != std::string_view::npos)
                                 ? line.substr(sp2 + 1, trailing - sp2 - 1)
                                 : line.substr(sp2 + 1);
  // Git writes a space before the trailing `@@`, so the new-range token can
  // carry trailing whitespace; trim both tokens before parsing them.
  while (!old_raw.empty() && (old_raw.back() == ' ' || old_raw.back() == '\t')) {
    old_raw.remove_suffix(1);
  }
  while (!new_raw.empty() && (new_raw.back() == ' ' || new_raw.back() == '\t')) {
    new_raw.remove_suffix(1);
  }
  const bool old_ok = parse_range_token(old_raw, old_start, old_len);
  const bool new_ok = parse_range_token(new_raw, new_start, new_len);
  return old_ok && new_ok;
}
}  // namespace

std::vector<DiffFile> UnifiedDiffParser::Parse(std::string_view text) const {
  std::vector<DiffFile> files;
  DiffFile* file = nullptr;
  DiffHunk* hunk = nullptr;
  int remaining_old = 0;
  int remaining_new = 0;
  int old_line = 0;
  int new_line = 0;

  std::string_view remaining = text;
  while (!remaining.empty()) {
    const std::size_t newline = remaining.find('\n');
    const std::string_view raw_line =
        (newline == std::string_view::npos) ? remaining : remaining.substr(0, newline);
    // Treat a trailing CR (from CRLF files) as line terminator, not content.
    std::string_view line = raw_line;
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    remaining =
        (newline == std::string_view::npos) ? std::string_view{} : remaining.substr(newline + 1);

    // A new file section.
    if (line.starts_with("diff --git ")) {
      files.push_back(DiffFile{});
      file = &files.back();
      hunk = nullptr;
      std::size_t pos = 11;  // len("diff --git ")
      file->old_path = path_from_token(next_word(line, pos), 'a');
      file->new_path = path_from_token(next_word(line, pos), 'b');
      continue;
    }

    // Header, hunk, and body lines only make sense inside a file section.
    if (file == nullptr) continue;

    // A hunk header begins a new hunk body; any prior hunk is complete.
    if (line.starts_with("@@")) {
      file->hunks.push_back(DiffHunk{});
      hunk = &file->hunks.back();
      hunk->header = std::string(line);
      old_line = 0;
      new_line = 0;
      remaining_old = 0;
      remaining_new = 0;
      if (!parse_hunk_header(line, old_line, remaining_old, new_line, remaining_new)) {
        // Malformed range: keep the raw header but consume no body lines.
        hunk = nullptr;
      }
      continue;
    }

    // Within a hunk body. This is handled before the `--- `/`+++ ` file-header
    // checks so that a deleted/added body line whose own text begins with `-- `
    // or `++ ` (e.g. a removed comment line `--- comment`) is not mistaken for
    // a file header and does not prematurely close the active hunk.
    if (hunk != nullptr) {
      if (line.empty()) {
        // A blank line / unexpected content inside the body closes the hunk.
        hunk = nullptr;
        continue;
      }
      const char marker = line.front();
      if (marker == '\\') {
        // "\ No newline at end of file" — informational, counts no line.
        continue;
      }
      if (marker == ' ' && remaining_old > 0 && remaining_new > 0) {
        DiffLine entry;
        entry.type = DiffLineType::kContext;
        entry.old_line = old_line;
        entry.new_line = new_line;
        entry.content = make_styled(line.substr(1));
        hunk->lines.push_back(std::move(entry));
        ++old_line;
        ++new_line;
        --remaining_old;
        --remaining_new;
      } else if (marker == '+' && remaining_new > 0) {
        DiffLine entry;
        entry.type = DiffLineType::kAdded;
        entry.new_line = new_line;
        entry.content = make_styled(line.substr(1));
        hunk->lines.push_back(std::move(entry));
        ++new_line;
        --remaining_new;
      } else if (marker == '-' && remaining_old > 0) {
        DiffLine entry;
        entry.type = DiffLineType::kDeleted;
        entry.old_line = old_line;
        entry.content = make_styled(line.substr(1));
        hunk->lines.push_back(std::move(entry));
        ++old_line;
        --remaining_old;
      } else {
        // Either the marker is unexpected or the hunk's declared counts are
        // exhausted; recover by closing the hunk.
        hunk = nullptr;
      }
      continue;
    }

    // Old/new file header lines only appear before hunks (never inside a hunk
    // body), so they are only recognized when no hunk is active.
    if (line.starts_with("--- ")) {
      file->old_path = dashed_path(line, "--- ");
      hunk = nullptr;
      continue;
    }
    if (line.starts_with("+++ ")) {
      file->new_path = dashed_path(line, "+++ ");
      hunk = nullptr;
      continue;
    }
    // Header noise (index, mode, similarity, Binary files lines, etc.) that
    // is neither a file header nor a hunk is ignored outside of a hunk body.
  }

  return files;
}

}  // namespace diff
}  // namespace terminal_ui_kit
