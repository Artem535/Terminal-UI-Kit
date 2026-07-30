#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace terminal_ui_kit {

/// Kind of a line inside a unified diff hunk.
enum class DiffLineKind {
  kContext,   ///< Unchanged line (space-prefixed)
  kAddition,  ///< New line (+-prefixed)
  kDeletion,  ///< Removed line (--prefixed)
  kUnknown,   ///< Unrecognised / malformed content
};

/// A single line inside a hunk.
struct DiffLine {
  DiffLineKind kind = DiffLineKind::kContext;
  std::string_view text;  ///< Content without the leading +/-/space prefix
  // old_lineno / new_lineno are reserved for future rendering support.
  // They are not currently populated by the parser (see Issue #31).
  std::uint32_t old_lineno = 0;  ///< Line number in the "from" file
  std::uint32_t new_lineno = 0;  ///< Line number in the "to" file
};

/// A contiguous hunk of diff lines.
struct DiffHunk {
  std::uint32_t old_start = 0;  ///< Starting old line number
  std::uint32_t old_count = 0;  ///< Number of lines from old file
  std::uint32_t new_start = 0;  ///< Starting new line number
  std::uint32_t new_count = 0;  ///< Number of lines from new file
  std::string context;          ///< @@ ... header text (unparsed)
  std::vector<DiffLine> lines;  ///< Hunk content
};

/// A single file diff.
struct DiffFile {
  std::string old_path;                           ///< Original file path
  std::string new_path;                           ///< New file path
  std::optional<std::string> change_description;  ///< e.g. "mode 100644 100755"
  std::optional<std::string> new_file_mode;
  std::optional<std::string> deleted_file_mode;
  bool new_is_binary = false;  ///< true for "Binary files X and Y differ"
  bool is_rename = false;
  bool is_copy = false;
  std::vector<DiffHunk> hunks;  ///< Empty for pure-file-ops / binary
};

/// Aggregated result of parsing a unified diff.
struct DiffDocument {
  std::vector<DiffFile> files;

  /// Total number of content diff lines across all hunks.
  [[nodiscard]] std::size_t total_line_count() const;

  /// Count of lines by kind.
  struct Counts {
    std::size_t additions = 0;
    std::size_t deletions = 0;
    std::size_t context = 0;
  };

  [[nodiscard]] Counts line_counts() const;
};

}  // namespace terminal_ui_kit
