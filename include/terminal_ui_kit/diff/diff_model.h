#pragma once

#include <optional>
#include <string>
#include <vector>

#include "terminal_ui_kit/core/styled_text.h"

namespace terminal_ui_kit {
namespace diff {

// Describes the role a single diff line plays in a hunk. File headers and
// hunk headers are stored separately (DiffFile::old_path/new_path and
// DiffHunk::header); a DiffLine is only ever a changed or context line.
enum class DiffLineType {
  // An unchanged line shown for context around the change.
  kContext,
  // A line added on the new side (`+` in the unified diff).
  kAdded,
  // A line removed from the old side (`-` in the unified diff).
  kDeleted,
};

// One line of a hunk. `old_line`/`new_line` carry the 1-based source-line
// numbers on the old and new side respectively; a nullopt means the line has
// no counterpart on that side (an added line has no old line, a deleted line
// has no new line). `content` is the line text after the leading diff marker
// character, so it is already clean of the `+`/`-`/space prefix.
struct DiffLine {
  DiffLineType type = DiffLineType::kContext;
  std::optional<int> old_line = std::nullopt;
  std::optional<int> new_line = std::nullopt;
  StyledText content;
};

// A contiguous `@@ -start,len +start,len @@` section of a file diff.
// `header` preserves the full hunk header line, including any trailing
// section heading text (e.g. a function name).
struct DiffHunk {
  std::string header;
  std::vector<DiffLine> lines;
};

// The complete diff for one file. For a new file `old_path` is `/dev/null`;
// for a deleted file `new_path` is `/dev/null`. A binary or unchanged file
// may have no hunks.
struct DiffFile {
  std::string old_path;
  std::string new_path;
  std::vector<DiffHunk> hunks;
};

}  // namespace diff
}  // namespace terminal_ui_kit
