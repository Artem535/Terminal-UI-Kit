#include "terminal_ui_kit/components/style_bridge.h"

#include "terminal_ui_kit/core/styled_text.h"
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

TEST(StyleBridge, RenderStyledTextEmptyReturnsEmpty) {
  StyledText text;
  ftxui::Element elem = render_styled_text(text);
  ftxui::Screen screen = test_support::render_to_screen(elem, 10, 1);
  EXPECT_EQ(screen.ToString(), "          ");
}

TEST(StyleBridge, RenderStyledTextBoldSpan) {
  StyledText text;
  TextStyle bold_style;
  bold_style.bold = true;
  text.append(TextSpan{"hello", bold_style});
  ftxui::Element elem = render_styled_text(text);
  ftxui::Screen screen = test_support::render_to_screen(elem, 10, 1);
  EXPECT_TRUE(screen.PixelAt(0, 0).bold);
  EXPECT_EQ(screen.PixelAt(0, 0).character, "h");
}

TEST(StyleBridge, RenderStyledTextMultipleSpans) {
  StyledText text;
  TextStyle bold_style;
  bold_style.bold = true;
  text.append(TextSpan{"ab", bold_style});
  text.append(TextSpan{"cd", TextStyle{}});
  ftxui::Screen screen = test_support::render_to_screen(render_styled_text(text), 10, 1);
  EXPECT_EQ(screen.PixelAt(0, 0).character, "a");
  EXPECT_EQ(screen.PixelAt(0, 0).bold, true);
  EXPECT_EQ(screen.PixelAt(1, 0).character, "b");
  EXPECT_EQ(screen.PixelAt(1, 0).bold, true);
  EXPECT_EQ(screen.PixelAt(2, 0).character, "c");
  EXPECT_EQ(screen.PixelAt(2, 0).bold, false);
  EXPECT_EQ(screen.PixelAt(3, 0).character, "d");
  EXPECT_EQ(screen.PixelAt(3, 0).bold, false);
}

}  // namespace
}  // namespace terminal_ui_kit
