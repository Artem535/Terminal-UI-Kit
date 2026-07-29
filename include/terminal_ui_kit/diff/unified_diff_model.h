#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "terminal_ui_kit/core/styled_text.h"

namespace terminal_ui_kit {

enum class DiffLineType { kContext, kAddition, kDeletion };

struct DiffLine {
  DiffLineType type = DiffLineType::kContext;
  std::optional<int> old_line;
  std::optional<int> new_line;
  StyledText content;
};

struct DiffHunk {
  std::string header;
  int old_start = 0;
  int old_count = 0;
  int new_start = 0;
  int new_count = 0;
  std::vector<DiffLine> lines;
};

struct DiffFile {
  std::string old_path;
  std::string new_path;
  bool is_new_file = false;
  bool is_deleted_file = false;
  bool is_binary = false;
  std::vector<DiffHunk> hunks;
};

class UnifiedDiffModel {
 public:
  void append_file(DiffFile file) { files_.push_back(std::move(file)); }

  [[nodiscard]] std::size_t file_count() const { return files_.size(); }
  [[nodiscard]] bool empty() const { return files_.empty(); }

  const DiffFile& file_at(std::size_t index) const { return files_.at(index); }
  std::vector<DiffFile>& files() { return files_; }
  const std::vector<DiffFile>& files() const { return files_; }

  void clear() { files_.clear(); }

 private:
  std::vector<DiffFile> files_;
};

}  // namespace terminal_ui_kit
