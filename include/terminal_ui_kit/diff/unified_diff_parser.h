#pragma once

#include <string>

#include "terminal_ui_kit/diff/diff_model.h"

namespace terminal_ui_kit {

/// @brief Unified diff parser.
///
/// Accepts raw unified diff text (e.g. output of `git diff` or
/// `diff -u`) and produces a structured model.
class UnifiedDiffParser {
 public:
  /// Parse the given unified diff text.
  /// @param raw Diff text, potentially containing multiple files.
  /// @returns Populated UnifiedDiffResult.
  static UnifiedDiffResult Parse(const std::string& raw);
};

}  // namespace terminal_ui_kit
