// Example: ToastView (PRD section 40)
//
// Interactive demo of the toast system: a ToastManager holds all retained
// state (queue, focus, timing via an injected clock), and the ToastView
// renders it and maps keyboard input. The view drives ToastManager::update()
// on every frame, so timed toasts expire and the queue drains under a normal
// ScreenInteractive loop -- nothing here creates its own event loop or
// background thread.
//
// Controls:
//   i          Add info toast
//   s          Add success toast
//   w          Add warning toast
//   e          Add error toast
//   a          Add toast with action
//   p          Add persistent toast (dismiss with Delete or c)
//   c          Clear all toasts
//   Tab        Move focus to next toast
//   Shift+Tab  Move focus to previous toast
//   Enter      Invoke the focused toast's action
//   Delete     Close the focused toast
//   t          Toggle color / no-color fallback
//   q or Esc   Quit
//
// Timed toasts show a live countdown; the focused toast's timeout is paused.
// Queueing and the visible-count limit are exercised by adding more toasts
// than max_visible.

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/key_hint_bar.h"
#include "terminal_ui_kit/components/toast_manager.h"
#include "terminal_ui_kit/components/toast_view.h"
#include "terminal_ui_kit/theme/theme.h"

int main() {
  using namespace std::chrono_literals;
  using namespace terminal_ui_kit;

  ToastManager manager(std::make_shared<SystemToastClock>(), /*max_visible=*/4);
  const Theme& theme = default_dark_theme();
  ToastView toast_view(manager, theme);
  bool color = true;

  auto show = [&](std::string message, ToastSeverity severity,
                  std::optional<std::chrono::milliseconds> timeout,
                  std::optional<ToastAction> action = std::nullopt) {
    manager.show(ToastOptions{std::move(message), severity, timeout, std::move(action)});
  };

  auto screen = ftxui::ScreenInteractive::Fullscreen();

  // Seed the tray so it is non-empty on startup; these drain via the clock.
  show("Build configured", ToastSeverity::kInfo, 8s);
  show("All tests passed", ToastSeverity::kSuccess, 8s);
  show("Persistent note \u2192 dismiss with Delete", ToastSeverity::kInfo, std::nullopt);

  ftxui::Component tray = toast_view.component();
  ftxui::Component root = ftxui::Renderer(tray, [&] {
    return ftxui::vbox({
               ftxui::text("Terminal UI Kit \u2014 Toast") | ftxui::bold,
               ftxui::text("Timed toasts tick and drain; the focused toast's "
                           "timeout pauses while focused.") |
                   ftxui::dim,
               ftxui::separator(),
               tray->Render(),
               ftxui::filler(),
               ftxui::text(color ? "color: on" : "color: off (no-color fallback)") | ftxui::dim,
               KeyHintBar({{"i", "info"},
                           {"s", "success"},
                           {"w", "warning"},
                           {"e", "error"},
                           {"a", "action"},
                           {"p", "persist"},
                           {"c", "clear"},
                           {"t", "color"},
                           {"tab", "focus"},
                           {"enter", "invoke"},
                           {"del", "close"},
                           {"q", "quit"}},
                          theme),
           }) |
           ftxui::border;
  });

  root = ftxui::CatchEvent(root, [&](ftxui::Event event) {
    using namespace std::chrono_literals;
    if (event == ftxui::Event::Character('i')) {
      show("Info notification", ToastSeverity::kInfo, 6s);
      return true;
    }
    if (event == ftxui::Event::Character('s')) {
      show("Build succeeded", ToastSeverity::kSuccess, 6s);
      return true;
    }
    if (event == ftxui::Event::Character('w')) {
      show("Disk space is running low", ToastSeverity::kWarning, 8s);
      return true;
    }
    if (event == ftxui::Event::Character('e')) {
      show("Connection lost; retrying", ToastSeverity::kError, 10s);
      return true;
    }
    if (event == ftxui::Event::Character('a')) {
      show("Task finished; an action is available", ToastSeverity::kInfo, 8s,
           ToastAction{"Undo", [] {}});
      return true;
    }
    if (event == ftxui::Event::Character('p')) {
      show("Persistent \u2014 close with Delete or c", ToastSeverity::kInfo, std::nullopt,
           ToastAction{"Dismiss", [] {}});
      return true;
    }
    if (event == ftxui::Event::Character('c')) {
      manager.clear_all();
      return true;
    }
    if (event == ftxui::Event::Character('t')) {
      color = !color;
      toast_view.set_color(color);
      return true;
    }
    if (event == ftxui::Event::Character('q') || event == ftxui::Event::Escape) {
      screen.ExitLoopClosure()();
      return true;
    }
    return false;
  });

  screen.Loop(root);
  return 0;
}