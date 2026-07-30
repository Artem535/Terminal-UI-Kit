#pragma once

#include <optional>
#include <string>
#include <vector>

namespace terminal_ui_kit {

// The type of a line in a unified diff (PRD section 28.3).
enum class DiffLineType {
  kContext,
  kAddition,
  kDeletion,
  kFileHeader,
  kHunkHeader,
  kBinary,
};

// A single line in a diff hunk (PRD section 28.3).
//
// `old_line` and `new_line` contain the 1-based line numbers in the old and
// new file respectively, or std::nullopt when the line type does not have a
// meaningful number (e.g. a file header).
struct DiffLine {
  DiffLineType type = DiffLineType::kContext;
  std::optional<int> old_line;
  std::optional<int> new_line;
  std::string content;

  friend bool operator==(const DiffLine&, const DiffLine&) = default;
};

// A contiguous block of changed lines in a unified diff (PRD section 28.3).
struct DiffHunk {
  // The raw hunk header text (e.g. "@@ -1,6 +1,7 @@ section").
  std::string header;
  // 1-based start line in the old file.
  int old_start = 0;
  // Number of lines the hunk covers in the old file.
  int old_count = 0;
  // 1-based start line in the new file.
  int new_start = 0;
  // Number of lines the hunk covers in the new file.
  int new_count = 0;
  std::vector<DiffLine> lines;

  friend bool operator==(const DiffHunk&, const DiffHunk&) = default;
};

// A single file's diff in a unified diff (PRD section 28.3).
struct DiffFile {
  std::string old_path;
  std::string new_path;
  bool is_binary = false;
  std::vector<DiffHunk> hunks;

  friend bool operator==(const DiffFile&, const DiffFile&) = default;
};

}  // namespace terminal_ui_kit
