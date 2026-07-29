#include "terminal_ui_kit/diff/unified_diff_parser.h"

#include <charconv>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace terminal_ui_kit {
namespace {

bool starts_with(std::string_view s, std::string_view prefix) {
  return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

std::string_view trim_left(std::string_view s) {
  std::size_t pos = 0;
  while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t')) {
    ++pos;
  }
  return s.substr(pos);
}

std::optional<int> parse_int(std::string_view s) {
  int value = 0;
  const char* first = s.data();
  const char* last = s.data() + s.size();
  auto [ptr, ec] = std::from_chars(first, last, value);
  if (ec != std::errc() || ptr != last) {
    return std::nullopt;
  }
  return value;
}

// Parse a hunk range like "-1,3" or "+5" into start and count.
// Default count is 1 if omitted.
struct HunkRange {
  int start = 0;
  int count = 0;
};

std::optional<HunkRange> parse_range(std::string_view text) {
  // text starts with '-' or '+'. Remove it.
  if (text.empty()) return std::nullopt;
  std::string_view num_part = text.substr(1);
  std::size_t comma = num_part.find(',');
  if (comma == std::string_view::npos) {
    auto start_opt = parse_int(num_part);
    if (!start_opt) return std::nullopt;
    return HunkRange{*start_opt, 1};
  }
  auto start_opt = parse_int(num_part.substr(0, comma));
  auto count_opt = parse_int(num_part.substr(comma + 1));
  if (!start_opt || !count_opt) return std::nullopt;
  return HunkRange{*start_opt, *count_opt};
}

enum class LineKind { kFileOld, kFileNew, kHunkHeader, kContext, kAddition, kDeletion, kOther };

LineKind classify_line(std::string_view line) {
  if (starts_with(line, "--- ")) return LineKind::kFileOld;
  if (starts_with(line, "+++ ")) return LineKind::kFileNew;
  if (starts_with(line, "@@")) return LineKind::kHunkHeader;
  if (!line.empty() && line[0] == ' ') return LineKind::kContext;
  if (!line.empty() && line[0] == '+') return LineKind::kAddition;
  if (!line.empty() && line[0] == '-') return LineKind::kDeletion;
  return LineKind::kOther;
}

// Extract path from a --- or +++ line, handling the optional timestamp part.
std::string extract_path(std::string_view line) {
  constexpr std::string_view kOldPrefix = "--- ";
  constexpr std::string_view kNewPrefix = "+++ ";
  if (starts_with(line, kOldPrefix)) {
    line = line.substr(kOldPrefix.size());
  } else if (starts_with(line, kNewPrefix)) {
    line = line.substr(kNewPrefix.size());
  }
  // Timestamp separator is a tab character.
  std::size_t tab = line.find('\t');
  if (tab != std::string_view::npos) {
    line = line.substr(0, tab);
  }
  return std::string(line);
}

struct ParserState {
  UnifiedDiffModel model;
  std::string error_message;
};

void flush_file(ParserState& state, std::optional<DiffFile>& current_file,
                std::optional<DiffHunk>& current_hunk) {
  if (current_hunk && current_file) {
    current_file->hunks.push_back(std::move(*current_hunk));
    current_hunk.reset();
  }
  if (current_file) {
    state.model.append_file(std::move(*current_file));
    current_file.reset();
  }
}

void flush_hunk(std::optional<DiffFile>& current_file, std::optional<DiffHunk>& current_hunk) {
  if (current_hunk && current_file) {
    current_file->hunks.push_back(std::move(*current_hunk));
    current_hunk.reset();
  }
}

bool parse_hunk_header(std::string_view line, DiffHunk& hunk) {
  // Format: @@ -old_start,old_count +new_start,new_count @@ ...
  if (!starts_with(line, "@@")) return false;
  std::size_t at2 = line.find("@@", 2);
  if (at2 == std::string_view::npos) return false;
  std::string_view range_part = trim_left(line.substr(2, at2 - 2));
  while (!range_part.empty() && (range_part.back() == ' ' || range_part.back() == '\t')) {
    range_part.remove_suffix(1);
  }
  // range_part looks like "-1,3 +4,5"
  std::size_t space = range_part.find(' ');
  if (space == std::string_view::npos) return false;
  auto old_range = parse_range(range_part.substr(0, space));
  auto new_range = parse_range(trim_left(range_part.substr(space + 1)));
  if (!old_range || !new_range) return false;
  hunk.old_start = old_range->start;
  hunk.old_count = old_range->count;
  hunk.new_start = new_range->start;
  hunk.new_count = new_range->count;
  std::size_t header_start = at2 + 2;
  if (header_start < line.size()) {
    hunk.header = std::string(trim_left(line.substr(header_start)));
  }
  return true;
}

}  // namespace

UnifiedDiffParseResult parse_unified_diff(std::string_view text) {
  ParserState state;
  std::optional<DiffFile> current_file;
  std::optional<DiffHunk> current_hunk;

  int old_line = 0;
  int new_line = 0;

  std::size_t pos = 0;
  while (pos < text.size()) {
    std::size_t end = text.find('\n', pos);
    if (end == std::string_view::npos) {
      end = text.size();
    }
    std::string_view line = text.substr(pos, end - pos);
    // Strip optional trailing \r for CRLF compatibility.
    if (!line.empty() && line.back() == '\r') {
      line.remove_suffix(1);
    }

    LineKind kind = classify_line(line);

    switch (kind) {
      case LineKind::kFileOld: {
        flush_file(state, current_file, current_hunk);
        DiffFile file;
        file.old_path = extract_path(line);
        // Detect binary-file header lines like "Binary files ... differ"
        if (file.old_path.find("/dev/null") != std::string::npos) {
          file.is_new_file = true;  // tentative, may be overridden
        }
        current_file = std::move(file);
        break;
      }
      case LineKind::kFileNew: {
        if (current_file) {
          current_file->new_path = extract_path(line);
          if (current_file->new_path.find("/dev/null") != std::string::npos) {
            current_file->is_deleted_file = true;
            current_file->is_new_file = false;
          } else if (current_file->old_path.find("/dev/null") != std::string::npos) {
            current_file->is_new_file = true;
            current_file->is_deleted_file = false;
          }
        }
        break;
      }
      case LineKind::kHunkHeader: {
        flush_hunk(current_file, current_hunk);
        if (!current_file) {
          state.error_message = "Hunk header without preceding file header";
          return UnifiedDiffParseResult{std::move(state.model), false,
                                        std::move(state.error_message)};
        }
        DiffHunk hunk;
        if (!parse_hunk_header(line, hunk)) {
          state.error_message = "Malformed hunk header";
          return UnifiedDiffParseResult{std::move(state.model), false,
                                        std::move(state.error_message)};
        }
        old_line = hunk.old_start;
        new_line = hunk.new_start;
        current_hunk = std::move(hunk);
        break;
      }
      case LineKind::kContext: {
        if (!current_hunk) {
          state.error_message = "Context line outside of hunk";
          return UnifiedDiffParseResult{std::move(state.model), false,
                                        std::move(state.error_message)};
        }
        DiffLine dline;
        dline.type = DiffLineType::kContext;
        dline.old_line = old_line;
        dline.new_line = new_line;
        dline.content.append(TextSpan{std::string(line.substr(1)), TextStyle{}, std::nullopt});
        current_hunk->lines.push_back(std::move(dline));
        ++old_line;
        ++new_line;
        break;
      }
      case LineKind::kAddition: {
        if (!current_hunk) {
          state.error_message = "Addition line outside of hunk";
          return UnifiedDiffParseResult{std::move(state.model), false,
                                        std::move(state.error_message)};
        }
        DiffLine dline;
        dline.type = DiffLineType::kAddition;
        dline.new_line = new_line;
        dline.content.append(TextSpan{std::string(line.substr(1)), TextStyle{}, std::nullopt});
        current_hunk->lines.push_back(std::move(dline));
        ++new_line;
        break;
      }
      case LineKind::kDeletion: {
        if (!current_hunk) {
          state.error_message = "Deletion line outside of hunk";
          return UnifiedDiffParseResult{std::move(state.model), false,
                                        std::move(state.error_message)};
        }
        DiffLine dline;
        dline.type = DiffLineType::kDeletion;
        dline.old_line = old_line;
        dline.content.append(TextSpan{std::string(line.substr(1)), TextStyle{}, std::nullopt});
        current_hunk->lines.push_back(std::move(dline));
        ++old_line;
        break;
      }
      case LineKind::kOther: {
        // Detect binary-file notices and treat them as a complete file.
        if (line.find("Binary files") != std::string_view::npos ||
            line.find("diff --git") != std::string_view::npos) {
          // diff --git lines start a new file group; we already handle files
          // through ---/+++, so ignore the git header.
          // Binary notices are informational; if we are inside a file, mark
          // it binary and stop parsing hunks for this file.
          if (current_file) {
            current_file->is_binary = true;
          }
        }
        // Any other unclassified line is ignored (e.g., "index ...").
        break;
      }
    }

    if (end == text.size()) break;
    pos = end + 1;
  }

  flush_file(state, current_file, current_hunk);

  if (state.model.empty() && !text.empty()) {
    state.error_message = "No valid diff content found";
    return UnifiedDiffParseResult{std::move(state.model), false, std::move(state.error_message)};
  }

  return UnifiedDiffParseResult{std::move(state.model), true, ""};
}

}  // namespace terminal_ui_kit
