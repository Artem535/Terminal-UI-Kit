#pragma once

#include <string>
#include <string_view>

#include "terminal_ui_kit/diff/diff_model.h"

namespace terminal_ui_kit {
namespace diff {

// Parse a unified-diff formatted string into a structured Diff.
//
// The parser is lenient: malformed hunks, missing headers, and unexpected
// content are tolerated where possible. Unknown lines are silently ignored.
Diff parse_unified_diff(std::string_view input);

}  // namespace diff
}  // namespace terminal_ui_kit
