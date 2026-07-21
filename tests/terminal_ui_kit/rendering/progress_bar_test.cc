#include "terminal_ui_kit/components/progress_bar.h"

#include <gtest/gtest.h>

#include "terminal_ui_kit/testing/virtual_screen.h"

namespace terminal_ui_kit {
namespace {

TEST(ProgressBar, RendersHalfFilledUnicodeBarAndPercentage) {
  ProgressBarOptions options;
  options.width = 4;

  ftxui::Screen screen = test_support::render_to_screen(
      ProgressBar(0.5, default_dark_theme(), options), 12, 1);
  std::string text;
  for (int column = 0; column < 12; ++column) {
    text += screen.PixelAt(column, 0).character;
  }

  EXPECT_NE(text.find("██░░"), std::string::npos);
  EXPECT_NE(text.find("50%"), std::string::npos);
}

TEST(ProgressBar, ClampsInvalidFractionToZero) {
  ProgressBarOptions options;
  options.width = 4;
  ftxui::Screen screen = test_support::render_to_screen(
      ProgressBar(-1.0, default_dark_theme(), options), 12, 1);

  EXPECT_EQ(screen.PixelAt(0, 0).character, "░");
  EXPECT_NE(screen.PixelAt(5, 0).character, "1");
}

TEST(ProgressBar, RendersEmptyElementForZeroWidth) {
  ProgressBarOptions options;
  options.width = 0;
  std::string text = test_support::render_to_text(ProgressBar(0.5, default_dark_theme(), options), 4, 1);

  EXPECT_EQ(text.find_first_not_of(' '), std::string::npos);
}

TEST(ProgressBar, RendersAsciiStyle) {
  ProgressBarOptions options;
  options.width = 4;
  options.style = ProgressStyle::kAscii;
  ftxui::Screen screen = test_support::render_to_screen(
      ProgressBar(0.5, default_dark_theme(), options), 12, 1);

  EXPECT_EQ(screen.PixelAt(0, 0).character, "#");
  EXPECT_EQ(screen.PixelAt(2, 0).character, "-");
}

TEST(ProgressBar, RendersDotsAndBrailleStyles) {
  ProgressBarOptions options;
  options.width = 2;
  options.style = ProgressStyle::kDots;
  ftxui::Screen dots = test_support::render_to_screen(
      ProgressBar(0.5, default_dark_theme(), options), 10, 1);
  EXPECT_EQ(dots.PixelAt(0, 0).character, "●");
  EXPECT_EQ(dots.PixelAt(1, 0).character, "○");

  options.style = ProgressStyle::kBraille;
  ftxui::Screen braille = test_support::render_to_screen(
      ProgressBar(0.5, default_dark_theme(), options), 10, 1);
  EXPECT_EQ(braille.PixelAt(0, 0).character, "⠿");
  EXPECT_EQ(braille.PixelAt(1, 0).character, "⠁");
}

TEST(ProgressBar, CanHidePercentage) {
  ProgressBarOptions options;
  options.width = 2;
  options.show_percentage = false;
  std::string text = test_support::render_to_text(ProgressBar(0.5, default_dark_theme(), options), 8, 1);

  EXPECT_EQ(text.find('%'), std::string::npos);
}

TEST(ProgressBar, NormalizesValueAndTotalAndUsesThemeRoles) {
  const Theme& theme = default_dark_theme();
  ProgressBarOptions options;
  options.width = 2;
  ftxui::Screen screen = test_support::render_to_screen(
      ProgressBar(1.0, 2.0, theme, options), 10, 1);

  ASSERT_TRUE(theme.accent.foreground.has_value());
  ASSERT_TRUE(theme.muted.foreground.has_value());
  EXPECT_EQ(screen.PixelAt(0, 0).foreground_color,
            ftxui::Color::RGB(theme.accent.foreground->red, theme.accent.foreground->green,
                              theme.accent.foreground->blue));
  EXPECT_EQ(screen.PixelAt(1, 0).foreground_color,
            ftxui::Color::RGB(theme.muted.foreground->red, theme.muted.foreground->green,
                              theme.muted.foreground->blue));
}

}  // namespace
}  // namespace terminal_ui_kit
