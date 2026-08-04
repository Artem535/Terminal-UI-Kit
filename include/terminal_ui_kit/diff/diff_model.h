#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "terminal_ui_kit/core/styled_text.h"

namespace terminal_ui_kit {
namespace diff {

// The role a single line plays in a unified diff (PRD section 28).
enum class DiffLineType {
  kContext,    // ' ' unchanged line present in both files
  kAddition,   // '+' added in the new file
  kDeletion,   // '-' removed from the old file
  kNoNewline,  // '\ No newline at end of file' marker
};

// One logical line of a unified diff, retaining its source text and, where
// applicable, its old-file and new-file line numbers (PRD section 28.3).
struct DiffLine {
  DiffLineType type = DiffLineType::kContext;
  std::optional<int> old_line;
  std::optional<int> new_line;
  StyledText content;

  friend bool operator==(const DiffLine&, const DiffLine&) = default;
};

// A hunk: a contiguous block of changed lines plus surrounding context.
// `header` keeps the raw '@@ ... @@' text, including any trailing function
// section that consumers may want to display.
struct DiffHunk {
  std::string header;
  // Old-file range as parsed from the header ('-start,count'), 1-based.
  int old_start = 0;
  int old_count = 0;
  // New-file range as parsed from the header ('+start,count'), 1-based.
  int new_start = 0;
  int new_count = 0;
  std::vector<DiffLine> lines;

  friend bool operator==(const DiffHunk&, const DiffHunk&) = default;
};

// A single file's diff: its paths, whether it was reported as binary, and
// the hunks it contains.
struct DiffFile {
  std::string old_path;
  std::string new_path;
  bool is_binary = false;
  std::vector<DiffHunk> hunks;

  friend bool operator==(const DiffFile&, const DiffFile&) = default;
};

// The retained result of parsing a unified diff: the ordered set of files
// it describes. Domain-neutral and free of any UI or terminal dependency.
struct DiffDocument {
  std::vector<DiffFile> files;

  friend bool operator==(const DiffDocument&, const DiffDocument&) = default;
};

}  // namespace diff
}  // namespace terminal_ui_kit
