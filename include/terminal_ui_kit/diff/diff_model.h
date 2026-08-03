#pragma once

#include <optional>
#include <string>
#include <vector>

namespace terminal_ui_kit {

/// @brief Type of a line within a unified diff hunk.
enum class DiffLineType {
  kContext,  /// Line present in both old and new file (starts with ' ').
  kAdded,    /// Line added in new file (starts with '+').
  kRemoved,  /// Line removed from old file (starts with '-').
};

/// @brief A single line inside a diff hunk.
struct DiffLine {
  DiffLineType type = DiffLineType::kContext;
  std::string text;

  /// Old-file line number (nullopt for additions).
  std::optional<int> old_line = std::nullopt;
  /// New-file line number (nullopt for deletions).
  std::optional<int> new_line = std::nullopt;
};

/// @brief A hunk within a diff (@@ block and following body lines).
struct DiffHunk {
  int old_start = 0;
  int old_count = 0;
  int new_start = 0;
  int new_count = 0;

  /// Optional context text after the trailing @@.
  std::string context;

  /// Lines belonging to this hunk in order.
  std::vector<DiffLine> lines;
};

/// @brief A single file represented in a unified diff.
struct DiffFile {
  /// Path in the old revision (empty for new files).
  std::string old_path;
  /// Path in the new revision (empty for deleted files).
  std::string new_path;

  /// True when the file is a binary diff (no hunks).
  bool is_binary = false;

  /// True when the old file had no trailing newline.
  bool old_no_newline = false;
  /// True when the new file has no trailing newline.
  bool new_no_newline = false;

  /// Parsed hunks (empty for binary files).
  std::vector<DiffHunk> hunks;
};

/// @brief Result of parsing a unified diff.
struct UnifiedDiffResult {
  /// Parsed files in order of appearance.
  std::vector<DiffFile> files;

  /// True if parsing completed without fatal errors.
  bool success = true;

  /// Human-readable description of the first encountered fatal problem.
  std::string error_message;
};

}  // namespace terminal_ui_kit
