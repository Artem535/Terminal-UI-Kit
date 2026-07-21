#include "terminal_ui_kit/components/style_bridge.h"

namespace terminal_ui_kit {

ftxui::Decorator to_decorator(const TextStyle& style) {
  ftxui::Decorator decorator = ftxui::nothing;

  if (style.bold) {
    decorator = decorator | ftxui::Decorator(ftxui::bold);
  }
  if (style.dim) {
    decorator = decorator | ftxui::Decorator(ftxui::dim);
  }
  if (style.underline) {
    decorator = decorator | ftxui::Decorator(ftxui::underlined);
  }
  if (style.strikethrough) {
    decorator = decorator | ftxui::Decorator(ftxui::strikethrough);
  }
  if (style.foreground) {
    const Color& foreground = *style.foreground;
    decorator = decorator |
                ftxui::color(ftxui::Color::RGB(foreground.red, foreground.green, foreground.blue));
  }
  if (style.background) {
    const Color& background = *style.background;
    decorator = decorator |
                ftxui::bgcolor(ftxui::Color::RGB(background.red, background.green, background.blue));
  }

  return decorator;
}

}  // namespace terminal_ui_kit
