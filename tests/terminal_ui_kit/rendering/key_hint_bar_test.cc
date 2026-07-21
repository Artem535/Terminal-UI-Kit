#include "terminal_ui_kit/components/key_hint_bar.h"

#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

std::string rendered_line(const ftxui::Screen& screen, int width) {
  std::string line;
  for (int x = 0; x < width; ++x) {
    line += screen.PixelAt(x, 0).character;
  }
  return line;
}

TEST(KeyHintBar, RendersHintsSeparatedByMiddleDot) {
  std::vector<KeyHint> hints = {
      {"esc", "cancel"},
      {"enter", "submit"},
  };

  ftxui::Element element = KeyHintBar(hints, default_dark_theme());
  ftxui::Screen screen = test_support::render_to_screen(element, 40, 1);

  EXPECT_NE(rendered_line(screen, 40).find("esc cancel · enter submit"), std::string::npos);
}

TEST(KeyHintBar, EmptyHintsRendersBlank) {
  ftxui::Element element = KeyHintBar({}, default_dark_theme());
  std::string text = test_support::render_to_text(element, 10, 1);

  EXPECT_EQ(text.find_first_not_of(' '), std::string::npos);
}

TEST(KeyHintBar, KeyLabelUsesAccentColor) {
  const Theme& theme = default_dark_theme();
  std::vector<KeyHint> hints = {{"q", "quit"}};

  ftxui::Element element = KeyHintBar(hints, theme);
  ftxui::Screen screen = test_support::render_to_screen(element, 10, 1);

  ASSERT_TRUE(theme.accent.foreground.has_value());
  const Color& expected = *theme.accent.foreground;
  EXPECT_EQ(screen.PixelAt(0, 0).foreground_color,
            ftxui::Color::RGB(expected.red, expected.green, expected.blue));
}

}  // namespace
}  // namespace terminal_ui_kit
