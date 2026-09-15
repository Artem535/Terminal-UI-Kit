#include "terminal_ui_kit/components/toast_view.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/animation.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/style_bridge.h"
#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {
namespace {

// Renders a timed toast's remaining time as a whole number of seconds, e.g.
// "5s". Never negative; rounded up so a brand-new toast shows its full second.
std::string Countdown(const std::chrono::steady_clock::duration remaining) {
  long long total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(remaining).count();
  if (total_ms < 0) {
    total_ms = 0;
  }
  const long long seconds = (total_ms + 999) / 1000;
  return std::to_string(seconds) + "s";
}

// One severity's leading glyph + text tag. The tag/icon are chosen so severity
// stays legible even when the theme has no foreground colors.
void SeverityVisual(ToastSeverity severity, const Theme& theme, const char** icon, const char** tag,
                    TextStyle* style) {
  switch (severity) {
    case ToastSeverity::kInfo:
      *icon = "i";
      *tag = "INFO";
      *style = theme.secondary;
      return;
    case ToastSeverity::kSuccess:
      *icon = "\u2713";  // ✓
      *tag = "OK";
      *style = theme.success;
      return;
    case ToastSeverity::kWarning:
      *icon = "\u25B2";  // ▲
      *tag = "WARN";
      *style = theme.warning;
      return;
    case ToastSeverity::kError:
      *icon = "\u2717";  // ✗
      *tag = "ERR";
      *style = theme.error;
      return;
  }
}

ftxui::Element RenderToast(const ToastInfo& toast, const Theme& theme) {
  const char* icon = nullptr;
  const char* tag = nullptr;
  TextStyle severity_style = theme.secondary;
  SeverityVisual(toast.severity, theme, &icon, &tag, &severity_style);

  ftxui::Elements line;
  // A left accent bar marks the focused toast; it is a glyph, so it stays
  // visible even when color is disabled.
  line.push_back(ftxui::text(toast.focused ? "\u258C" : " ") |
                 (toast.focused ? ftxui::bold : ftxui::nothing));
  line.push_back(ftxui::text(std::string(icon) + " " + tag) | to_decorator(severity_style));
  line.push_back(ftxui::text(" "));

  ftxui::Element message =
      ftxui::paragraph(toast.message) | ftxui::flex | to_decorator(theme.primary);
  if (toast.focused) {
    message = message | ftxui::bold;
  }
  line.push_back(std::move(message));

  if (!toast.action_label.empty()) {
    line.push_back(ftxui::text(" [") | ftxui::dim);
    line.push_back(ftxui::text(toast.action_label) | to_decorator(theme.accent));
    line.push_back(ftxui::text("]") | ftxui::dim);
  }
  if (!toast.persistent) {
    line.push_back(ftxui::text("  " + Countdown(toast.remaining)) | ftxui::dim);
  }

  return ftxui::window(ftxui::text(""), ftxui::hbox(std::move(line)));
}

}  // namespace

class ToastViewImpl : public ftxui::ComponentBase {
 public:
  ToastViewImpl(ToastManager& manager, const Theme& theme)
      : manager_(manager),
        color_theme_(theme),
        no_color_theme_(terminal_ui_kit::without_color(theme)),
        color_(true) {}

  void set_color(bool enabled) { color_ = enabled; }
  bool color() const { return color_; }

 private:
  ftxui::Element Render() override {
    manager_.update();
    const Theme& theme = color_ ? color_theme_ : no_color_theme_;
    bool has_timed = false;
    for (const ToastInfo& toast : manager_.visible()) {
      if (!toast.persistent) {
        has_timed = true;
        break;
      }
    }
    // Keep the frame refreshing while a timed toast is still ticking.
    if (has_timed) {
      ftxui::animation::RequestAnimationFrame();
    }
    return ToastElement(manager_, theme);
  }

  bool Focusable() const override { return true; }

  bool OnEvent(ftxui::Event event) override {
    if (!Active()) {
      return false;
    }
    if (event == ftxui::Event::Tab) {
      manager_.move_focus(1);
      return true;
    }
    if (event == ftxui::Event::TabReverse) {
      manager_.move_focus(-1);
      return true;
    }
    if (event == ftxui::Event::Return) {
      // Only consume Enter when the focused toast has an invocable action;
      // otherwise let the key fall through.
      return manager_.invoke_focused();
    }
    if (event == ftxui::Event::Delete || event == ftxui::Event::Backspace) {
      const std::optional<std::uint64_t> id = manager_.focused_id();
      if (id.has_value()) {
        manager_.close(*id);
        return true;
      }
      return false;
    }
    return false;
  }

  ToastManager& manager_;
  Theme color_theme_;
  Theme no_color_theme_;
  bool color_;
};

ftxui::Element ToastElement(const ToastManager& manager, const Theme& theme) {
  const std::vector<ToastInfo> toasts = manager.visible();
  if (toasts.empty()) {
    return ftxui::text("");
  }
  ftxui::Elements rows;
  rows.reserve(toasts.size());
  for (const ToastInfo& toast : toasts) {
    rows.push_back(RenderToast(toast, theme));
  }
  return ftxui::vbox(std::move(rows));
}

ToastView::ToastView(ToastManager& manager, const Theme& theme)
    : impl_(ftxui::Make<ToastViewImpl>(manager, theme)) {}

ftxui::Component ToastView::component() const { return impl_; }

void ToastView::set_color(bool enabled) { impl_->set_color(enabled); }

bool ToastView::color() const { return impl_->color(); }

}  // namespace terminal_ui_kit