#pragma once

#include <optional>
#include <string>
#include <vector>

#include "terminal_ui_kit/core/styled_text.h"

namespace terminal_ui_kit::diff {

// The kind of a changed line within a hunk (PRD section 28.3). File headers
// and hunk headers are carried as strings on DiffFile/DiffHunk rather than
// as DiffLine entries, so the line type only distinguishes the three line
// kinds that appear inside a hunk body.
enum class DiffLineType {
  kContext,
  kAddition,
  kDeletion,
};

// A single line of a unified diff hunk. `old_line`/`new_line` are the
// 1-based line numbers in the old/new file, present only when the line
// exists on that side (deletions have no new line, additions have no old
// line, context lines have both).
struct DiffLine {
  DiffLineType type = DiffLineType::kContext;
  std::optional<int> old_line;
  std::optional<int> new_line;
  StyledText content;
};

// A contiguous changed region of a file, introduced by a `@@ ... @@` header.
struct DiffHunk {
  std::string header;
  std::vector<DiffLine> lines;
};

// One file's worth of changes in a unified diff. `old_path`/`new_path` come
// from the `---`/`+++` headers (with any trailing timestamp stripped and the
// git `a/`/`b/` prefix removed, so paths are stored in bare form, matching
// `rename from`/`rename to` and `diff -u` output); `is_binary` is set when
// the diff carries a "Binary files ... differ" notice instead of hunks.
struct DiffFile {
  std::string old_path;
  std::string new_path;
  bool is_binary = false;
  std::vector<DiffHunk> hunks;
};

// The result of parsing a unified diff: an ordered collection of files.
struct DiffDocument {
  std::vector<DiffFile> files;
};

}  // namespace terminal_ui_kit::diff
