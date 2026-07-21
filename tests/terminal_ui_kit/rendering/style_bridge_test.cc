#include "terminal_ui_kit/components/style_bridge.h"

#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TEST(StyleBridge, AppliesForegroundColor) {
  TextStyle style;
  style.foreground = Color{10, 20, 30};

  ftxui::Element element = ftxui::text("x") | to_decorator(style);
  ftxui::Screen screen = test_support::render_to_screen(element, 1, 1);

  EXPECT_EQ(screen.PixelAt(0, 0).foreground_color, ftxui::Color::RGB(10, 20, 30));
}

TEST(StyleBridge, AppliesBackgroundColor) {
  TextStyle style;
  style.background = Color{40, 50, 60};

  ftxui::Element element = ftxui::text("x") | to_decorator(style);
  ftxui::Screen screen = test_support::render_to_screen(element, 1, 1);

  EXPECT_EQ(screen.PixelAt(0, 0).background_color, ftxui::Color::RGB(40, 50, 60));
}

TEST(StyleBridge, AppliesBoldDimUnderlineStrikethrough) {
  TextStyle style;
  style.bold = true;
  style.dim = true;
  style.underline = true;
  style.strikethrough = true;

  ftxui::Element element = ftxui::text("x") | to_decorator(style);
  ftxui::Screen screen = test_support::render_to_screen(element, 1, 1);

  const ftxui::Pixel& pixel = screen.PixelAt(0, 0);
  EXPECT_TRUE(pixel.bold);
  EXPECT_TRUE(pixel.dim);
  EXPECT_TRUE(pixel.underlined);
  EXPECT_TRUE(pixel.strikethrough);
}

TEST(StyleBridge, DefaultStyleAppliesNoDecoration) {
  TextStyle style;

  ftxui::Element element = ftxui::text("x") | to_decorator(style);
  ftxui::Screen screen = test_support::render_to_screen(element, 1, 1);

  const ftxui::Pixel& pixel = screen.PixelAt(0, 0);
  EXPECT_FALSE(pixel.bold);
  EXPECT_FALSE(pixel.dim);
  EXPECT_FALSE(pixel.underlined);
  EXPECT_FALSE(pixel.strikethrough);
  EXPECT_EQ(pixel.foreground_color, ftxui::Color::Default);
  EXPECT_EQ(pixel.background_color, ftxui::Color::Default);
}

}  // namespace
}  // namespace terminal_ui_kit
