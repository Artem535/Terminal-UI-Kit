#pragma once

#include "terminal_ui_kit/core/text_position.h"

namespace terminal_ui_kit {

// A half-open [start, end) span between two TextPosition values (PRD
// section 13.2). `end` is exclusive, matching typical editor selection
// semantics.
struct TextRange {
  TextPosition start;
  TextPosition end;

  [[nodiscard]] bool contains(const TextPosition& position) const {
    return position >= start && position < end;
  }

  [[nodiscard]] bool is_empty() const { return start == end; }
};

}  // namespace terminal_ui_kit
