#pragma once

#include <string>
#include <string_view>

#include "terminal_ui_kit/diff/unified_diff_model.h"

namespace terminal_ui_kit {

struct UnifiedDiffParseResult {
  UnifiedDiffModel model;
  bool success = false;
  std::string error_message;
};

// Parses a unified diff text into a model.
// On malformed input, returns an error result with a diagnostic message.
[[nodiscard]] UnifiedDiffParseResult parse_unified_diff(std::string_view text);

}  // namespace terminal_ui_kit
