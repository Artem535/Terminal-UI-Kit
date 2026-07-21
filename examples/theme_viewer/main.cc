#include <string>
#include <vector>

#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include "terminal_ui_kit/components/style_bridge.h"
#include "terminal_ui_kit/theme/theme.h"

namespace {

using terminal_ui_kit::TextStyle;
using terminal_ui_kit::Theme;

struct RoleDemo {
  std::string name;
  TextStyle Theme::* field;
  std::string sample;
};

const std::vector<RoleDemo>& roles() {
  static const std::vector<RoleDemo> demo = {
      {"primary", &Theme::primary, "Sample text"},
      {"secondary", &Theme::secondary, "Sample text"},
      {"muted", &Theme::muted, "Sample text"},
      {"success", &Theme::success, "Sample text"},
      {"warning", &Theme::warning, "Sample text"},
      {"error", &Theme::error, "Sample text"},
      {"accent", &Theme::accent, "Sample text"},
      {"code", &Theme::code, "Sample text"},
      {"addition", &Theme::addition, "+ added line"},
      {"deletion", &Theme::deletion, "- removed line"},
      {"border", &Theme::border, "Sample text"},
      {"selected", &Theme::selected, "Sample text"},
      {"focused", &Theme::focused, "Sample text"},
  };
  return demo;
}

}  // namespace

int main() {
  bool dark = true;
  bool color_on = true;

  auto screen = ftxui::ScreenInteractive::Fullscreen();

  auto renderer = ftxui::Renderer([&] {
    Theme base =
        dark ? terminal_ui_kit::default_dark_theme() : terminal_ui_kit::default_light_theme();
    Theme theme = color_on ? base : terminal_ui_kit::without_color(base);

    ftxui::Elements rows;
    for (const auto& role : roles()) {
      const TextStyle& style = theme.*(role.field);
      rows.push_back(ftxui::hbox({
          ftxui::text(role.name) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12),
          ftxui::text(role.sample) | terminal_ui_kit::to_decorator(style),
      }));
    }

    std::string status = std::string("Theme: ") + (dark ? "Dark" : "Light") +
                         " | Color: " + (color_on ? "On" : "Off");

    return ftxui::vbox({
               ftxui::text("Terminal UI Kit - Theme Viewer") | ftxui::bold,
               ftxui::text("[t] toggle dark/light   [c] toggle color   [q] quit"),
               ftxui::separator(),
               ftxui::vbox(std::move(rows)),
               ftxui::separator(),
               ftxui::text(status),
           }) |
           ftxui::border;
  });

  renderer |= ftxui::CatchEvent([&](ftxui::Event event) {
    if (event == ftxui::Event::Character("t")) {
      dark = !dark;
      return true;
    }
    if (event == ftxui::Event::Character("c")) {
      color_on = !color_on;
      return true;
    }
    if (event == ftxui::Event::Character("q") || event == ftxui::Event::Escape) {
      screen.ExitLoopClosure()();
      return true;
    }
    return false;
  });

  screen.Loop(renderer);
  return 0;
}
