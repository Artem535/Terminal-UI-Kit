#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/key_hint_bar.h"
#include "terminal_ui_kit/components/progress_tree.h"
#include "terminal_ui_kit/theme/theme.h"

int main() {
  using namespace terminal_ui_kit;
  const Theme& theme = default_dark_theme();
  ProgressTask configure{"configure", "Configure", Status::kSuccess, 1.0, "0.4s", {}};
  ProgressTask compile{"compile", "Compile", Status::kRunning, 0.65, "", {}};
  ProgressTask tests{"tests", "Tests", Status::kRunning, std::nullopt, "", {}};
  ProgressTask build{"build", "Build workflow",           Status::kRunning, std::nullopt,
                     "",      {configure, compile, tests}};
  ftxui::Component tree = ProgressTree({build}, theme);
  ftxui::Component root = ftxui::Renderer(tree, [&] {
    return ftxui::vbox({
               ftxui::text("Terminal UI Kit - Task Dashboard") | ftxui::bold,
               ftxui::text("ProgressTree with immutable task snapshots.") | ftxui::dim,
               ftxui::separator(),
               tree->Render(),
               ftxui::filler(),
               KeyHintBar({{"up/down", "select"},
                           {"space/enter", "toggle"},
                           {"home/end", "jump"},
                           {"q", "quit"}},
                          theme),
           }) |
           ftxui::border;
  });
  auto screen = ftxui::ScreenInteractive::Fullscreen();
  root |= ftxui::CatchEvent([&](ftxui::Event event) {
    if (event == ftxui::Event::Character('q')) {
      screen.ExitLoopClosure()();
      return true;
    }
    return false;
  });
  screen.Loop(root);
}
