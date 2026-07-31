#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace terminal_ui_kit {
namespace diff {

// Represents the type of a change in a unified diff.
enum class LineType { kContext, kAddition, kDeletion };

// Represents a single line within a hunk of a unified diff.
struct DiffLine {
  LineType type;
  std::string content;  // Text without the leading +/-/space marker.
};

// Represents a single contiguous hunk (@@ ... @@ block).
struct Hunk {
  // Start line number in the old file.
  std::uint32_t old_start;
  // Number of lines in the old file the hunk covers.
  std::uint32_t old_length;
  // Start line number in the new file.
  std::uint32_t new_start;
  // Number of lines in the new file the hunk covers.
  std::uint32_t new_length;

  // Hunk header as it appeared in the diff
  // ("@@ -old_start,length +new_start,length @@ ...").
  std::string header;

  std::vector<DiffLine> lines;
};

// Represents a single file diff.
struct DiffFile {
  // Path in the old file; "/dev/null" when the file was created.
  std::string old_path;
  // Path in the new file; "/dev/null" when the file was deleted.
  std::string new_path;

  // Original line from the diff, e.g. "diff --git a/foo b/bar".
  std::string header;

  // true when the file is binary ("Binary files … differ").
  bool is_binary = false;

  std::vector<Hunk> hunks;
};

// Root: the parsed result, which may contain zero or more file diffs.
struct Diff {
  std::vector<DiffFile> files;
};

}  // namespace diff
}  // namespace terminal_ui_kit
