#include "terminal_ui_kit/components/status_indicator.h"

#include <utility>

#include "terminal_ui_kit/components/style_bridge.h"

namespace terminal_ui_kit {

ftxui::Element StatusIndicator(Status status, std::string text, const Theme& theme) {
  const char* icon = "?";
  TextStyle icon_style;
  bool blink = false;

  switch (status) {
    case Status::kIdle:
      icon = "○";
      icon_style = theme.muted;
      break;
    case Status::kPending:
      icon = "…";
      icon_style = theme.muted;
      break;
    case Status::kRunning:
      icon = "●";
      icon_style = theme.accent;
      blink = true;
      break;
    case Status::kSuccess:
      icon = "✓";
      icon_style = theme.success;
      break;
    case Status::kWarning:
      icon = "▲";
      icon_style = theme.warning;
      break;
    case Status::kError:
      icon = "✗";
      icon_style = theme.error;
      break;
    case Status::kCancelled:
      icon = "⊘";
      icon_style = theme.muted;
      break;
    case Status::kPaused:
      icon = "❚❚";
      icon_style = theme.warning;
      break;
  }

  ftxui::Element icon_element = ftxui::text(icon) | to_decorator(icon_style);
  if (blink) {
    icon_element = icon_element | ftxui::blink;
  }

  return ftxui::hbox({
      std::move(icon_element),
      ftxui::text(" "),
      ftxui::text(std::move(text)) | to_decorator(theme.primary),
  });
}

}  // namespace terminal_ui_kit
