#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "terminal_ui_kit/diff/diff_model.h"

namespace terminal_ui_kit {

// Parse result for a single attempt at processing unified-diff text.
struct DiffParseResult {
  // Files successfully parsed from the input.
  std::vector<DiffFile> files;
  // Line number (1-based) at which the parser encountered an error, or
  // std::nullopt if the input was parsed completely without errors.
  std::optional<std::size_t> error_line;
  // Human-readable description of the error, if any.
  std::string error_message;
};

// Parses a complete unified-diff string into a list of DiffFile objects.
//
// The parser handles:
//   - File headers (--- a/path, +++ b/path)
//   - Hunk headers (@@ -old,new +new,count @@ ...)
//   - Context lines (leading space)
//   - Addition lines (leading +)
//   - Deletion lines (leading -)
//   - Binary file notices
//   - Multiple files in a single diff
//
// Malformed input is handled gracefully: the parser skips unrecognised
// lines and reports the first error location via DiffParseResult::error_line
// but still returns any files that were successfully parsed before the
// error.
//
// The parser is domain-neutral and has no dependency on FTXUI or any
// terminal rendering backend.
[[nodiscard]] DiffParseResult parse_unified_diff(std::string_view input);

}  // namespace terminal_ui_kit
