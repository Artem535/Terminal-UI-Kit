#pragma once

#include <string_view>
#include <vector>

#include "terminal_ui_kit/diff/diff_model.h"

namespace terminal_ui_kit {
namespace diff {

// A small uniformed-diff parser that turns a pre-generated unified diff
// (typically from `git diff --no-color`) into a retained, display-agnostic
// model (PRD section 28). It performs presentation-layer parsing only: it
// never generates diffs and does not depend on FTXUI or any terminal
// backend, so it can be used by pure-data consumers and tests alike.
//
// The parser is a stateless value type; Parse() may be called any number of
// times. Malformed or incomplete input never throws and is recovered as
// predictably as possible (see Parse() for the exact rules).
class UnifiedDiffParser {
 public:
  // Parses a unified diff containing zero or more `diff --git` file
  // sections and returns one DiffFile per section, in encounter order.
  //
  // Recovery rules for malformed/partial input:
  //  * Content before the first `diff --git` header is ignored.
  //  * A section without a parseable hunk range still yields a DiffFile
  //    (its hunks are simply empty).
  //  * A hunk body that ends before its declared line counts (truncated
  //    diff) keeps whatever lines were seen.
  //  * `\ No newline at end of file` marker lines are dropped.
  [[nodiscard]] std::vector<DiffFile> Parse(std::string_view text) const;
};

}  // namespace diff
}  // namespace terminal_ui_kit
