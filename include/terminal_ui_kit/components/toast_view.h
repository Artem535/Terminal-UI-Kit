#pragma once

#include <memory>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/toast_manager.h"
#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

// Renders a non-interactive snapshot of the manager's visible toasts. Each
// toast is a full-width bordered row with a severity prefix, message, optional
// action label, and a live countdown for timed toasts. Severity is conveyed by
// both color (when the theme has color) and a plain-text tag/icon so the
// render stays legible under `Theme::without_color` as well.
ftxui::Element ToastElement(const ToastManager& manager, const Theme& theme);

class ToastViewImpl;

// Interactive toast tray. It holds no state of its own beyond the theme; all
// retained state lives in the `ToastManager` passed to the constructor, which
// must outlive the view. Rendering calls `ToastManager::update()` each frame so
// timed toasts expire under a live `ScreenInteractive` loop without an event
// loop or background thread of its own. focus, queueing and actions are wholly
// delegated to the manager.
class ToastView {
 public:
  explicit ToastView(ToastManager& manager, const Theme& theme);

  ftxui::Component component() const;

  // Enables/disables color. When color is off the render uses
  // `Theme::without_color(theme)`; severity stays legible via tags/icons and
  // the focus marker.
  void set_color(bool enabled);
  [[nodiscard]] bool color() const;

 private:
  std::shared_ptr<ToastViewImpl> impl_;
};

}  // namespace terminal_ui_kit