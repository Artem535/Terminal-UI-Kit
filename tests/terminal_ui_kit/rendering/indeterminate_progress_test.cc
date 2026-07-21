#include "terminal_ui_kit/components/indeterminate_progress.h"

#include <ftxui/component/animation.hpp>

#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TEST(IndeterminateProgress, IsNotFocusableAndRendersInitialSegment) {
  ProgressBarOptions options;
  options.width = 4;
  ftxui::Component progress = IndeterminateProgress(default_dark_theme(), options, 2);

  EXPECT_FALSE(progress->Focusable());
  ftxui::Screen screen = test_support::render_to_screen(progress->Render(), 4, 1);
  EXPECT_EQ(screen.PixelAt(0, 0).character, "█");
  EXPECT_EQ(screen.PixelAt(2, 0).character, "░");
}

TEST(IndeterminateProgress, AdvancesSegmentAfterFrameDuration) {
  ProgressBarOptions options;
  options.width = 4;
  ftxui::Component progress =
      IndeterminateProgress(default_dark_theme(), options, 2, std::chrono::milliseconds(50));
  ftxui::animation::Params params(std::chrono::milliseconds(60));
  progress->OnAnimation(params);

  ftxui::Screen screen = test_support::render_to_screen(progress->Render(), 4, 1);
  EXPECT_EQ(screen.PixelAt(0, 0).character, "░");
  EXPECT_EQ(screen.PixelAt(1, 0).character, "█");
}

TEST(IndeterminateProgress, WrapsSegmentAtRightEdge) {
  ProgressBarOptions options;
  options.width = 3;
  ftxui::Component progress =
      IndeterminateProgress(default_dark_theme(), options, 1, std::chrono::milliseconds(10));
  ftxui::animation::Params params(std::chrono::milliseconds(30));
  progress->OnAnimation(params);

  ftxui::Screen screen = test_support::render_to_screen(progress->Render(), 3, 1);
  EXPECT_EQ(screen.PixelAt(0, 0).character, "█");
}

TEST(IndeterminateProgress, WrapsEveryCellOfWideSegment) {
  ProgressBarOptions options;
  options.width = 4;
  ftxui::Component progress =
      IndeterminateProgress(default_dark_theme(), options, 2, std::chrono::milliseconds(10));
  ftxui::animation::Params params(std::chrono::milliseconds(30));
  progress->OnAnimation(params);

  ftxui::Screen screen = test_support::render_to_screen(progress->Render(), 4, 1);
  EXPECT_EQ(screen.PixelAt(3, 0).character, "█");
  EXPECT_EQ(screen.PixelAt(0, 0).character, "█");
}

TEST(IndeterminateProgress, ClampsNonPositiveFrameDuration) {
  ProgressBarOptions options;
  options.width = 2;
  ftxui::Component progress =
      IndeterminateProgress(default_dark_theme(), options, 1, std::chrono::milliseconds(0));
  ftxui::animation::Params params(std::chrono::milliseconds(1));

  progress->OnAnimation(params);
  EXPECT_FALSE(progress->Focusable());
}

}  // namespace
}  // namespace terminal_ui_kit
