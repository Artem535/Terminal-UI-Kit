#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "terminal_ui_kit/core/styled_text.h"

namespace terminal_ui_kit {

// Line types in unified diff (PRD section 28.3).
enum class DiffLineType {
  kContext,     // Unchanged line (prefix ' ')
  kAddition,    // Added line (prefix '+')
  kDeletion,    // Deleted line (prefix '-')
  kFileHeader,  // Index/file header lines
  kSourcePath,  // Source path line (prefix '---')
  kTargetPath,  // Target path line (prefix '+++')
  kHunkHeader,  // Hunk header (prefix '@@')
};

// Single diff line structure (PRD section 28.3).
struct DiffLine {
  DiffLineType type;
  std::optional<int> old_line = std::nullopt;
  std::optional<int> new_line = std::nullopt;
  StyledText content;
};

// Single hunk structure (change block) (PRD section 28.3).
struct DiffHunk {
  std::string header;  // Text of header like @@ -old_start,count +new_start,count @@
  std::vector<DiffLine> lines;
};

// Single file diff structure (PRD section 28.3).
struct DiffFile {
  std::string old_path;  // Path to old file
  std::string new_path;  // Path to new file
  std::vector<DiffHunk> hunks;
};

}  // namespace terminal_ui_kit