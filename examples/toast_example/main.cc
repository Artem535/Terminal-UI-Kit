// Toast Viewer example for the Terminal UI Kit Toast component.
//
// Demonstrates: every severity, queueing and the max-visible limit, timed and
// persistent toasts, action callbacks, keyboard focus with timeout pause,
// no-color fallback, clear-all and terminal resize. All sample data is
// deterministic (no network, no RNG).
//
// Controls:
//   i            Add an info toast
//   s            Add a success toast
//   w            Add a warning toast
//   e            Add an error toast
//   a            Add a toast with an action ("Revert")
//   p            Add a persistent toast (never expires)
//   c            Clear all toasts
//   n            Toggle no-color mode
//   Tab          Move focus to the next toast
//   Shift+Tab    Move focus to the previous toast
//   Enter        Invoke the focused toast's action
//   Delete       Close the focused toast
//   q / Esc      Exit
#include <chrono>
#include <cstddef>
#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/key_hint_bar.h"
#include "terminal_ui_kit/components/toast.h"
#include "terminal_ui_kit/theme/theme.h"

namespace {

using terminal_ui_kit::ToastAction;
using terminal_ui_kit::ToastSeverity;

constexpr std::chrono::milliseconds kDefaultDuration = std::chrono::seconds(8);

}  // namespace

int main() {
  using namespace terminal_ui_kit;
  using namespace ftxui;

  const Theme& theme = default_dark_theme();

  ToastManagerOptions manager_options;
  manager_options.max_visible = 4;
  ToastManager manager(manager_options);

  // Demonstrates no-color behaviour: toggle live with 'n'.
  bool no_color = false;
  int action_count = 0;  // shown in the reverted-toast feedback
  ToastViewOptions view_options;
  view_options.max_toast_width = 60;
  auto toast_view = Make<ToastView>(manager, theme, view_options);
  Component toast_component = toast_view;  // base pointer (public Render)

  auto screen = ScreenInteractive::Fullscreen();

  auto add = [&](ToastOptions options) { manager.Show(std::move(options)); };

  Component root = Renderer(toast_component, [&] {
    const std::size_t visible = manager.visible_count();
    const std::size_t queued = manager.queued_count();

    Element toasts = toast_component->Render();
    if (visible == 0) {
      toasts = text("(no toasts - press a key above to add one)") | dim;
    }

    std::string status = "visible: " + std::to_string(visible) +
                         "  queued: " + std::to_string(queued) +
                         "  mode: " + (no_color ? "no-color" : "color");
    if (manager.HasFocus()) {
      status += "  [focus paused]";
    }
    if (action_count > 0) {
      status += "  actions fired: " + std::to_string(action_count);
    }

    return vbox({
               text("Terminal UI Kit - Toast Viewer") | bold,
               text("Reusable toasts with queueing, actions, focus pause and no-color.") | dim,
               separator(),
               text(status) | bold,
               separator(),
               toasts | flex,
               separator(),
               text("Controls") | bold,
               KeyHintBar({{"i", "info"},
                           {"s", "success"},
                           {"w", "warning"},
                           {"e", "error"},
                           {"a", "action"},
                           {"p", "persistent"},
                           {"c", "clear all"},
                           {"n", "no-color"}},
                          theme),
               KeyHintBar({{"Tab/Shift+Tab", "focus"},
                           {"Enter", "action"},
                           {"Delete", "close"},
                           {"q/Esc", "quit"}},
                          theme),
           }) |
           border;
  });

  root |= CatchEvent([&](Event event) {
    if (event == Event::Character('i')) {
      ToastOptions options;
      options.message = "Info: indexing 42 items";
      options.severity = ToastSeverity::kInfo;
      options.duration = kDefaultDuration;
      add(std::move(options));
      return true;
    }
    if (event == Event::Character('s')) {
      ToastOptions options;
      options.message = "Success: build completed in 1.2s";
      options.severity = ToastSeverity::kSuccess;
      options.duration = kDefaultDuration;
      add(std::move(options));
      return true;
    }
    if (event == Event::Character('w')) {
      ToastOptions options;
      options.message = "Warning: disk usage above 80%";
      options.severity = ToastSeverity::kWarning;
      options.duration = kDefaultDuration;
      add(std::move(options));
      return true;
    }
    if (event == Event::Character('e')) {
      ToastOptions options;
      options.message = "Error: failed to connect to server";
      options.severity = ToastSeverity::kError;
      options.duration = kDefaultDuration;
      add(std::move(options));
      return true;
    }
    if (event == Event::Character('a')) {
      ToastOptions options;
      options.message = "Config overwritten on disk";
      options.severity = ToastSeverity::kWarning;
      options.duration = kDefaultDuration;
      options.action = ToastAction{"Revert",
                                   // Capturing main-scope locals by reference is safe: the manager
                                   // and its callbacks live for the whole main() scope.
                                   [&action_count] { ++action_count; }};
      add(std::move(options));
      return true;
    }
    if (event == Event::Character('p')) {
      ToastOptions options;
      options.message = "Persistent: you must acknowledge this";
      options.severity = ToastSeverity::kError;
      options.duration = std::nullopt;  // never expires
      add(std::move(options));
      return true;
    }
    if (event == Event::Character('c')) {
      manager.ClearAll();
      return true;
    }
    if (event == Event::Character('n')) {
      no_color = !no_color;
      toast_view->SetNoColor(no_color);
      return true;
    }
    if (event == Event::Character('q') || event == Event::Escape) {
      screen.ExitLoopClosure()();
      return true;
    }
    return false;
  });

  screen.Loop(root);

  return 0;
}