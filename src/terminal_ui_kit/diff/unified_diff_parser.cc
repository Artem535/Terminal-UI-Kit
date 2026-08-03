#include "terminal_ui_kit/diff/unified_diff_parser.h"

#include <cctype>
#include <optional>
#include <string>
#include <string_view>

namespace terminal_ui_kit {

namespace {

constexpr std::string_view kDiffGitPrefix = "diff --git ";
constexpr std::string_view kIndexPrefix = "index ";
constexpr std::string_view kSimilarityPrefix = "similarity index ";
constexpr std::string_view kRenameFromPrefix = "rename from ";
constexpr std::string_view kRenameToPrefix = "rename to ";
constexpr std::string_view kNewFileMode = "new file mode ";
constexpr std::string_view kDeletedFileMode = "deleted file mode ";
constexpr std::string_view kBinaryFilesPrefix = "Binary files ";
constexpr std::string_view kOldPathPrefix = "--- ";
constexpr std::string_view kNewPathPrefix = "+++ ";
constexpr std::string_view kHunkPrefix = "@@";
constexpr std::string_view kNoNewlineOld = "\\ No newline at end of file";
constexpr std::string_view kNoNewlineNew = "\\ No newline at end of file";

std::string NormalizePath(std::string_view path) {
  if (!path.empty() && path.front() == '"' && path.back() == '"') {
    path.remove_prefix(1);
    path.remove_suffix(1);
  }
  if (path.size() > 2 && path[0] == 'a' && path[1] == '/') {
    path.remove_prefix(2);
  } else if (path.size() > 2 && path[0] == 'b' && path[1] == '/') {
    path.remove_prefix(2);
  }
  return std::string(path);
}

// Extract old and new paths from a `diff --git a/<old> b/<new>` line.
bool ExtractGitDiffPaths(std::string_view line, std::string& old_path, std::string& new_path) {
  if (line.size() < kDiffGitPrefix.size() + 8 ||
      line.substr(0, kDiffGitPrefix.size()) != kDiffGitPrefix) {
    return false;
  }

  // Find the " a/" and " b/" delimiters that git inserts.
  auto sep_a = line.find(" a/");
  auto sep_b = line.find(" b/");

  // Check for quoted paths (needed when paths contain spaces).
  if (sep_a == std::string_view::npos) {
    sep_a = line.find(" \"a/");
  }
  if (sep_b == std::string_view::npos) {
    sep_b = line.find(" \"b/");
  }

  if (sep_a == std::string_view::npos || sep_b == std::string_view::npos || sep_a >= sep_b) {
    return false;
  }

  old_path = NormalizePath(line.substr(sep_a + 3, sep_b - sep_a - 3));
  new_path = NormalizePath(line.substr(sep_b + 3));
  return true;
}

// Parse a range token such as `-10,3`, `+5`, `,5`, or `-0,0`.
bool ParseRangeToken(std::string_view token, int& start, int& count) {
  std::size_t i = 0;
  if (i < token.size() && (token[i] == '+' || token[i] == '-')) {
    ++i;
  }
  const std::size_t number_begin = i;
  while (i < token.size() && token[i] >= '0' && token[i] <= '9') {
    ++i;
  }
  const bool has_start = i > number_begin;
  start = 0;
  count = 1;  // default for single-line hunks where ",1" is omitted
  if (has_start) {
    int v = 0;
    for (std::size_t k = number_begin; k < i; ++k) {
      v = v * 10 + (token[k] - '0');
    }
    start = v;
  }
  if (i < token.size()) {
    if (token[i] != ',') {
      return false;
    }
    ++i;
    const std::size_t length_begin = i;
    while (i < token.size() && token[i] >= '0' && token[i] <= '9') {
      ++i;
    }
    if (i > length_begin) {
      int v = 0;
      for (std::size_t k = length_begin; k < i; ++k) {
        v = v * 10 + (token[k] - '0');
      }
      count = v;
    }
    if (i != token.size()) {
      return false;  // trailing junk
    }
  } else if (!has_start) {
    return false;  // empty token
  }
  return true;
}

// Parse hunk header `@@ -old_start,old_length +new_start,new_length @@ context`
bool ParseHunkHeader(std::string_view line, int& old_start, int& old_count, int& new_start,
                     int& new_count, std::string& context) {
  if (line.size() < 4 || line.substr(0, 2) != kHunkPrefix) {
    return false;
  }

  const std::size_t sp1 = line.find(' ', 2);
  if (sp1 == std::string_view::npos) {
    return false;
  }
  const std::size_t sp2 = line.find(' ', sp1 + 1);
  if (sp2 == std::string_view::npos) {
    return false;
  }

  std::string_view old_raw = line.substr(sp1 + 1, sp2 - sp1 - 1);
  const std::size_t trailing = line.find("@@", sp2 + 1);
  std::string_view new_raw = (trailing != std::string_view::npos)
                                 ? line.substr(sp2 + 1, trailing - sp2 - 1)
                                 : line.substr(sp2 + 1);

  // Trim trailing whitespace from both tokens.
  while (!old_raw.empty() && (old_raw.back() == ' ' || old_raw.back() == '\t')) {
    old_raw.remove_suffix(1);
  }
  while (!new_raw.empty() && (new_raw.back() == ' ' || new_raw.back() == '\t')) {
    new_raw.remove_suffix(1);
  }

  if (!ParseRangeToken(old_raw, old_start, old_count) ||
      !ParseRangeToken(new_raw, new_start, new_count)) {
    return false;
  }

  if (trailing != std::string_view::npos && trailing + 2 + 1 < line.size()) {
    context = std::string(line.substr(trailing + 3));
  }

  return true;
}

}  // namespace

UnifiedDiffResult UnifiedDiffParser::Parse(const std::string& raw) {
  UnifiedDiffResult result;
  result.success = true;

  std::vector<std::string_view> lines;
  lines.reserve(256);

  std::size_t pos = 0;
  while (pos < raw.size()) {
    std::size_t nl = raw.find('\n', pos);
    if (nl == std::string::npos) {
      if (pos == raw.size()) {
        break;
      }
      std::string_view tail(raw.data() + pos, raw.size() - pos);
      if (!tail.empty() && tail.back() == '\r') {
        tail.remove_suffix(1);
      }
      if (!tail.empty()) {
        lines.push_back(tail);
      }
      break;
    }
    std::string_view line(raw.data() + pos, nl - pos);
    if (!line.empty() && line.back() == '\r') {
      line.remove_suffix(1);
    }
    if (!line.empty()) {
      lines.push_back(line);
    }
    pos = nl + 1;
  }

  DiffFile* file = nullptr;
  DiffHunk* hunk = nullptr;
  int remaining_old = 0;
  int remaining_new = 0;
  int current_old_line = 0;
  int current_new_line = 0;

  for (const std::string_view line : lines) {
    // 1. diff --git starts a new file section.
    if (line.size() >= kDiffGitPrefix.size() &&
        line.substr(0, kDiffGitPrefix.size()) == kDiffGitPrefix) {
      result.files.emplace_back();
      file = &result.files.back();
      hunk = nullptr;
      remaining_old = remaining_new = 0;
      std::string old_path_from_git;
      std::string new_path_from_git;
      if (ExtractGitDiffPaths(line, old_path_from_git, new_path_from_git)) {
        file->old_path = std::move(old_path_from_git);
        file->new_path = std::move(new_path_from_git);
      }
      continue;
    }

    // Guard: no active file and not a diff --git header -> skip.
    if (file == nullptr) {
      continue;
    }

    // 2. Hunk header inside an active file.
    if (line.size() >= 4 && line.substr(0, 2) == kHunkPrefix) {
      file->hunks.emplace_back();
      hunk = &file->hunks.back();
      if (!ParseHunkHeader(line, hunk->old_start, hunk->old_count, hunk->new_start, hunk->new_count,
                           hunk->context)) {
        // Keep raw unparsed line as context; reset counters to avoid
        // unbounded body consumption.
        hunk->context = std::string(line);
        hunk->old_start = hunk->old_count = 0;
        hunk->new_start = hunk->new_count = 0;
      }
      remaining_old = hunk->old_count;
      remaining_new = hunk->new_count;
      current_old_line = hunk->old_start;
      current_new_line = hunk->new_start;
      continue;
    }

    // 3. Hunk body lines must be checked BEFORE --- / +++ file headers
    //    so that body lines starting with "--- " or "+++ " are not
    //    mis-interpreted as file headers.
    if (hunk != nullptr && remaining_old > 0 && remaining_new > 0) {
      if (line.empty() || line[0] == ' ') {
        // Strip the single leading space that unified diff uses as
        // the context-line marker.  If the line is exactly " " it
        // represents an empty original line.
        std::string content = line.empty() ? std::string() : std::string(line.substr(1));
        hunk->lines.push_back(DiffLine{DiffLineType::kContext, std::move(content), current_old_line,
                                       current_new_line});
        ++current_old_line;
        ++current_new_line;
        --remaining_old;
        --remaining_new;
        continue;
      }
    }
    if (hunk != nullptr && remaining_new > 0) {
      if (!line.empty() && line[0] == '+') {
        hunk->lines.push_back(DiffLine{DiffLineType::kAdded, std::string(line.substr(1)),
                                       std::nullopt, current_new_line});
        ++current_new_line;
        --remaining_new;
        continue;
      }
    }
    if (hunk != nullptr && remaining_old > 0) {
      if (!line.empty() && line[0] == '-') {
        hunk->lines.push_back(DiffLine{DiffLineType::kRemoved, std::string(line.substr(1)),
                                       current_old_line, std::nullopt});
        ++current_old_line;
        --remaining_old;
        continue;
      }
    }
    // If there was an active hunk but the line did not match within bounds,
    // close the hunk.
    if (hunk != nullptr) {
      hunk = nullptr;
    }

    // 4. No active hunk: interpret --- / +++ as file headers.
    if (line.size() >= kOldPathPrefix.size() &&
        line.substr(0, kOldPathPrefix.size()) == kOldPathPrefix) {
      file->old_path = NormalizePath(line.substr(kOldPathPrefix.size()));
      continue;
    }
    if (line.size() >= kNewPathPrefix.size() &&
        line.substr(0, kNewPathPrefix.size()) == kNewPathPrefix) {
      file->new_path = NormalizePath(line.substr(kNewPathPrefix.size()));
      continue;
    }

    // 5. Binary file detection.
    if (line.size() >= kBinaryFilesPrefix.size() &&
        line.substr(0, kBinaryFilesPrefix.size()) == kBinaryFilesPrefix) {
      file->is_binary = true;
      continue;
    }

    // 6. No-newline markers.
    //    Git emits "\ No newline at end of file" after the last hunk line.
    if (line == kNoNewlineOld || line == kNoNewlineNew) {
      if (file->hunks.empty()) {
        file->old_no_newline = true;
      } else {
        DiffHunk& last_hunk = file->hunks.back();
        if (!last_hunk.lines.empty()) {
          const DiffLineType last_type = last_hunk.lines.back().type;
          if (last_type == DiffLineType::kRemoved || last_type == DiffLineType::kContext) {
            file->old_no_newline = true;
          }
          if (last_type == DiffLineType::kAdded || last_type == DiffLineType::kContext) {
            file->new_no_newline = true;
          }
        }
      }
      continue;
    }

    // Any other line inside a file section is ignored.
  }

  return result;
}

}  // namespace terminal_ui_kit
