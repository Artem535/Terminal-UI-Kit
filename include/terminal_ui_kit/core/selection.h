#pragma once

#include <optional>
#include <string>

#include "terminal_ui_kit/core/text_position.h"
#include "terminal_ui_kit/core/text_range.h"

namespace terminal_ui_kit {

class StreamingDocument;

class SelectionManager {
 public:
  SelectionManager() = default;

  void start(TextPosition pos);
  void extend_to(TextPosition pos);
  void clear();
  [[nodiscard]] bool has_selection() const;
  [[nodiscard]] TextRange range() const;
  [[nodiscard]] bool is_selecting() const;

  [[nodiscard]] std::string selected_text(const StreamingDocument& doc) const;

 private:
  std::optional<TextPosition> anchor_;
  std::optional<TextPosition> active_;
  bool selecting_ = false;
};

}  // namespace terminal_ui_kit
