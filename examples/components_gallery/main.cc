#include <cstddef>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/collapsible_panel.h"
#include "terminal_ui_kit/components/key_hint_bar.h"
#include "terminal_ui_kit/components/modal_stack.h"
#include "terminal_ui_kit/components/spinner.h"
#include "terminal_ui_kit/components/status.h"
#include "terminal_ui_kit/components/status_indicator.h"
#include "terminal_ui_kit/components/style_bridge.h"
#include "terminal_ui_kit/theme/theme.h"

namespace {

using terminal_ui_kit::CollapsiblePanel;
using terminal_ui_kit::CollapsiblePanelOptions;
using terminal_ui_kit::KeyHintBar;
using terminal_ui_kit::ModalStack;
using terminal_ui_kit::Spinner;
using terminal_ui_kit::Status;
using terminal_ui_kit::StatusIndicator;
using terminal_ui_kit::Theme;

const std::vector<Status>& all_statuses() {
  static const std::vector<Status> statuses = {
      Status::kIdle,    Status::kPending, Status::kRunning,   Status::kSuccess,
      Status::kWarning, Status::kError,   Status::kCancelled, Status::kPaused,
  };
  return statuses;
}

const char* status_name(Status status) {
  switch (status) {
    case Status::kIdle:
      return "idle";
    case Status::kPending:
      return "pending";
    case Status::kRunning:
      return "running";
    case Status::kSuccess:
      return "success";
    case Status::kWarning:
      return "warning";
    case Status::kError:
      return "error";
    case Status::kCancelled:
      return "cancelled";
    case Status::kPaused:
      return "paused";
  }
  return "unknown";
}

}  // namespace

int main() {
  const Theme& theme = terminal_ui_kit::default_dark_theme();
  std::size_t status_index = 0;

  ftxui::Component spinner = Spinner();
  ftxui::Component panel_body = ftxui::Renderer([&theme] {
    return ftxui::vbox({
        StatusIndicator(Status::kSuccess, "lint", theme),
        StatusIndicator(Status::kSuccess, "build", theme),
        StatusIndicator(Status::kError, "tests", theme),
    });
  });

  CollapsiblePanelOptions panel_options;
  panel_options.title = "Build output";
  panel_options.status = Status::kWarning;
  panel_options.summary = "2 passed, 1 failed";
  panel_options.duration_text = "1.4s";
  ftxui::Component panel = CollapsiblePanel(panel_options, panel_body, theme);

  ftxui::Component base = ftxui::Renderer(
      ftxui::Container::Vertical({spinner, panel}), [&theme, spinner, panel, &status_index] {
        Status status = all_statuses()[status_index];
        return ftxui::vbox({
                   ftxui::text("Terminal UI Kit - Components Gallery") | ftxui::bold,
                   ftxui::separator(),
                   StatusIndicator(status, status_name(status), theme),
                   ftxui::text(""),
                   spinner->Render(),
                   ftxui::text(""),
                   panel->Render(),
                   ftxui::filler(),
                   KeyHintBar({{"s", "cycle status"}, {"m", "open modal"}, {"q", "quit"}}, theme),
               }) |
               ftxui::border;
      });

  ModalStack modal_stack(base);
  ftxui::Component modal_dialog = ftxui::Renderer([&theme] {
    return ftxui::vbox({
               ftxui::text("Confirm action?") | terminal_ui_kit::to_decorator(theme.primary),
               ftxui::separator(),
               KeyHintBar({{"y", "confirm"}, {"esc", "cancel"}}, theme),
           }) |
           ftxui::border;
  });

  auto screen = ftxui::ScreenInteractive::Fullscreen();
  ftxui::Component root = modal_stack.component();
  root |= ftxui::CatchEvent([&](ftxui::Event event) {
    if (!modal_stack.empty()) {
      if (event == ftxui::Event::Character('y') || event == ftxui::Event::Escape) {
        modal_stack.pop();
        return true;
      }
      return false;
    }

    if (event == ftxui::Event::Character('s')) {
      status_index = (status_index + 1) % all_statuses().size();
      return true;
    }
    if (event == ftxui::Event::Character('m')) {
      modal_stack.push(modal_dialog);
      return true;
    }
    if (event == ftxui::Event::Character('q')) {
      screen.ExitLoopClosure()();
      return true;
    }
    return false;
  });

  screen.Loop(root);
  return 0;
}
