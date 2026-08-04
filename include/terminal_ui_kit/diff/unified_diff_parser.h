#pragma once

#include <string_view>

#include "terminal_ui_kit/diff/diff_model.h"

namespace terminal_ui_kit {
namespace diff {

// Parses a unified diff document (as produced by `git diff` or GNU diff -u)
// into a retained DiffDocument model.
//
// The parser is intentionally lenient: malformed hunks, truncated headers,
// and unknown metadata lines are skipped rather than aborting the parse, so
// that a partially readable diff still yields a usable model. Lines inside a
// hunk are their own authoritative source of line numbers, so the parser
// tracks each line positionally and always fills old_line/new_line for
// context, addition, and deletion lines; the hunk-level ranges parsed from
// the header are retained for display.
//
// Returns the parsed document. On a completely empty or unparseable input
// the returned document contains no files.
DiffDocument parse_unified_diff(std::string_view text);

}  // namespace diff
}  // namespace terminal_ui_kit
