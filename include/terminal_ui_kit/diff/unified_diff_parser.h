#pragma once

#include <string_view>

#include "terminal_ui_kit/diff/diff_model.h"

namespace terminal_ui_kit::diff {

// Parses a unified diff (as produced by `git diff` or `diff -u`) into a
// retained DiffDocument. The parser is domain-neutral and has no FTXUI
// dependency (PRD section 28.2). Malformed input is handled predictably:
// unrecognized lines outside hunks are ignored, malformed hunk headers are
// skipped, and content lines outside a hunk are dropped.
DiffDocument parse_unified_diff(std::string_view text);

}  // namespace terminal_ui_kit::diff
