#include "terminal_ui_kit/components/key_hint_bar.h"

#include <cstddef>
#include <utility>

#include "terminal_ui_kit/components/style_bridge.h"

namespace terminal_ui_kit {

ftxui::Element KeyHintBar(std::vector<KeyHint> hints, const Theme& theme) {
  ftxui::Elements row;

  for (std::size_t i = 0; i < hints.size(); ++i) {
    if (i > 0) {
      row.push_back(ftxui::text(" · ") | to_decorator(theme.muted));
    }
    row.push_back(ftxui::text(hints[i].key_label) | to_decorator(theme.accent));
    row.push_back(ftxui::text(" "));
    row.push_back(ftxui::text(hints[i].action) | to_decorator(theme.secondary));
  }

  return ftxui::hbox(std::move(row));
}

}  // namespace terminal_ui_kit
