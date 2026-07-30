#pragma once

#include <string_view>

#include "terminal_ui_kit/diff/diff_model.h"

namespace terminal_ui_kit {

/// Parse unified diff / patch text into a DiffDocument.
///
/// Handles:
/// - --- / +++ file headers (with optional a/ b/ prefixes)
/// - @@ hunk headers with old/new line ranges and optional context
/// - +, -, space prefixed hunk content lines
/// - "Binary files X and Y differ" notices
/// - rename / copy / mode-change metadata
///
/// On parse error or empty input an empty DiffDocument is returned.
[[nodiscard]] DiffDocument parse_unified_diff(std::string_view input);

}  // namespace terminal_ui_kit
