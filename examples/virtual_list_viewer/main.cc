#include <cstddef>
#include <string>
#include <utility>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/key_hint_bar.h"
#include "terminal_ui_kit/components/virtual_list.h"
#include "terminal_ui_kit/theme/theme.h"

int main() {
  using namespace terminal_ui_kit;

  constexpr std::size_t kItemCount = 100000;
  std::size_t rendered_this_frame = 0;
  VirtualListOptions options;
  options.item_count = [] { return kItemCount; };
  options.render_item = [&rendered_this_frame](std::size_t index, int) {
    ++rendered_this_frame;
    return ftxui::text("Row " + std::to_string(index));
  };
  VirtualListModel model(std::move(options));
  ftxui::Component list = model.component();

  auto screen = ftxui::ScreenInteractive::Fullscreen();
  ftxui::Component root = ftxui::Renderer(list, [&] {
    rendered_this_frame = 0;
    ftxui::Element list_element = list->Render();
    return ftxui::vbox({
               ftxui::text("Terminal UI Kit - VirtualList Viewer") | ftxui::bold,
               ftxui::text("100,000 fixed-height rows; only the viewport is rendered.") |
                   ftxui::dim,
               ftxui::separator(),
               ftxui::text("Virtual window") | ftxui::bold,
               list_element | ftxui::flex,
               ftxui::separator(),
               ftxui::text("Rendered this frame: " + std::to_string(rendered_this_frame)) |
                   ftxui::dim,
               ftxui::text("Controls") | ftxui::bold,
               KeyHintBar({{"up/down", "select"},
                           {"pgup/pgdn", "page"},
                           {"home/end", "jump"},
                           {"q", "quit"}},
                          default_dark_theme()),
           }) |
           ftxui::border;
  });
  root |= ftxui::CatchEvent([&](ftxui::Event event) {
    if (event == ftxui::Event::Character('q')) {
      screen.ExitLoopClosure()();
      return true;
    }
    return false;
  });
  screen.Loop(root);
}
