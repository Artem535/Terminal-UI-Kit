#include "terminal_ui_kit/components/status_indicator.h"

#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TEST(StatusIndicator, RendersIconAndText) {
  ftxui::Element element = StatusIndicator(Status::kSuccess, "Build passed", default_dark_theme());
  std::string text = test_support::render_to_text(element, 40, 1);

  EXPECT_NE(text.find("✓"), std::string::npos);
  EXPECT_NE(text.find("Build passed"), std::string::npos);
}

TEST(StatusIndicator, ErrorUsesErrorForegroundColor) {
  const Theme& theme = default_dark_theme();
  ftxui::Element element = StatusIndicator(Status::kError, "Failed", theme);
  ftxui::Screen screen = test_support::render_to_screen(element, 40, 1);

  ASSERT_TRUE(theme.error.foreground.has_value());
  const Color& expected = *theme.error.foreground;
  EXPECT_EQ(screen.PixelAt(0, 0).foreground_color,
            ftxui::Color::RGB(expected.red, expected.green, expected.blue));
}

TEST(StatusIndicator, RunningStatusBlinks) {
  ftxui::Element element = StatusIndicator(Status::kRunning, "Building", default_dark_theme());
  ftxui::Screen screen = test_support::render_to_screen(element, 40, 1);

  EXPECT_TRUE(screen.PixelAt(0, 0).blink);
}

TEST(StatusIndicator, IdleStatusDoesNotBlink) {
  ftxui::Element element = StatusIndicator(Status::kIdle, "Waiting", default_dark_theme());
  ftxui::Screen screen = test_support::render_to_screen(element, 40, 1);

  EXPECT_FALSE(screen.PixelAt(0, 0).blink);
}

}  // namespace
}  // namespace terminal_ui_kit
