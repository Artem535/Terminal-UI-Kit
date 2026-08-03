#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace terminal_ui_kit {

/**
 * Minimal, domain‑neutral representation of a Unified Diff.
 *
 * The model is deliberately lightweight – it contains only the data
 * required by the rendering component (which will be added later) and by
 * the parser tests.  No UI or FTXUI types are used here.
 */

// Represents a line inside a hunk.
struct DiffLine {
  enum class Type { Context, Added, Removed, NoNewline } type;
  // The raw text of the line **without** the leading diff marker (+/‑/ ).
  std::string text;
  // 1‑based line numbers from the original (old) and new file. May be empty
  // when the line does not belong to the respective side (e.g. an added line
  // has no old line number).
  std::optional<std::size_t> old_line;
  std::optional<std::size_t> new_line;
};

// Represents a single hunk of changes for a file.
struct DiffHunk {
  std::size_t old_start = 0;   // 1‑based line number in the old file
  std::size_t old_lines = 0;   // number of lines the hunk spans in old file
  std::size_t new_start = 0;   // 1‑based line number in the new file
  std::size_t new_lines = 0;   // number of lines the hunk spans in new file
  std::vector<DiffLine> lines;
};

// Represents a diff for a single file (including possible binary diff).
struct DiffFile {
  std::string old_path;   // as reported after "---"
  std::string new_path;   // as reported after "+++"
  bool is_binary = false; // true when a binary differ notice is present
  std::vector<DiffHunk> hunks;
};

// Stateless parser – just a namespace with a free function.
class UnifiedDiffParser {
 public:
  /**
   * Parse a unified diff string.
   *
   * The function never throws; malformed input results in an empty vector or
   * partially parsed files.  Consumers are expected to handle an empty result as
   * "no diff".
   */
  static std::vector<DiffFile> Parse(const std::string& diff_text);
};

}  // namespace terminal_ui_kit
