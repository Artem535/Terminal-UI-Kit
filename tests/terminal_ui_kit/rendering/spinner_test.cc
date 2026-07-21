#include "terminal_ui_kit/components/spinner.h"

#include <chrono>

#include <ftxui/component/animation.hpp>

#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TEST(Spinner, IsNotFocusable) {
  ftxui::Component spinner = Spinner();
  EXPECT_FALSE(spinner->Focusable());
}

TEST(Spinner, RendersFirstFrameOfCharsetTwo) {
  ftxui::Component spinner = Spinner(2);
  std::string text = test_support::render_to_text(spinner->Render(), 5, 1);

  EXPECT_NE(text.find('|'), std::string::npos);
}

TEST(Spinner, AdvancesFrameAfterElapsedDurationExceedsThreshold) {
  ftxui::Component spinner = Spinner(2, std::chrono::milliseconds(50));

  std::string before = test_support::render_to_text(spinner->Render(), 5, 1);

  ftxui::animation::Params params(std::chrono::milliseconds(60));
  spinner->OnAnimation(params);

  std::string after = test_support::render_to_text(spinner->Render(), 5, 1);

  EXPECT_NE(before, after);
}

TEST(Spinner, DoesNotAdvanceFrameBeforeThreshold) {
  ftxui::Component spinner = Spinner(2, std::chrono::milliseconds(50));

  std::string before = test_support::render_to_text(spinner->Render(), 5, 1);

  ftxui::animation::Params params(std::chrono::milliseconds(10));
  spinner->OnAnimation(params);

  std::string after = test_support::render_to_text(spinner->Render(), 5, 1);

  EXPECT_EQ(before, after);
}

}  // namespace
}  // namespace terminal_ui_kit
