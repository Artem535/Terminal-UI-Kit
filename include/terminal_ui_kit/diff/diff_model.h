#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace terminal_ui_kit {

enum class DiffChangeType {
  kAddition,
  kDeletion,
  kContext,
  kFileHeader,
  kHunkHeader,
  kBinary,
  kMode,
  kIndex,
  kCommitInfo,
  kUnknown
};

enum class DiffFileType {
  kNormal,
  kBinary,
  kNewFile,
  kDeletedFile,
  kRenamedFile,
  kCopyFile
};

// A single line within a unified diff, carrying both structural
// (file-header, hunk-header) and data (add, delete, context) roles.
struct DiffLine {
  DiffChangeType change_type = DiffChangeType::kUnknown;
  std::optional<int> old_line_no;
  std::optional<int> new_line_no;
  std::string text;
};

// A contiguous hunk of diff lines with header metadata.
struct DiffHunk {
  int old_start = 0;
  int old_count = 0;
  int new_start = 0;
  int new_count = 0;
  std::vector<DiffLine> lines;

  [[nodiscard]] int total_lines() const {
    return static_cast<int>(lines.size());
  }
};

// Metadata from a mode-change line (e.g. "old mode 755  new mode 644").
struct DiffModeChange {
  std::string old_mode;
  std::string new_mode;
};

// A single file within a multi-file diff.
struct DiffFile {
  std::string old_path;
  std::string new_path;
  DiffFileType type = DiffFileType::kNormal;
  std::vector<DiffModeChange> mode_changes;
  std::vector<DiffHunk> hunks;
};

// Top-level container returned after parsing a complete unified diff.
class DiffModel {
 public:
  std::vector<DiffFile> files;

  // Parse a unified diff string into a model. Returns an empty model on
  // malformed input; parse errors are logged to stderr.
  static DiffModel parse(std::string_view input);

  [[nodiscard]] std::size_t file_count() const { return files.size(); }
  [[nodiscard]] std::size_t hunk_count() const;
  [[nodiscard]] std::size_t line_count() const;

  // Iterators over all diff lines across all files.
  [[nodiscard]] std::size_t addition_count() const;
  [[nodiscard]] std::size_t deletion_count() const;
  [[nodiscard]] std::size_t context_count() const;
};

}  // namespace terminal_ui_kit
