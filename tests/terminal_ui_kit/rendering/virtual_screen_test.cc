#include "terminal_ui_kit/testing/virtual_screen.h"

#include <ftxui/dom/elements.hpp>
#include <gtest/gtest.h>

namespace terminal_ui_kit::test_support {
namespace {

TEST(RenderToText, RendersPlainTextPaddedToWidth) {
  auto element = ftxui::text("hi");
  EXPECT_EQ(render_to_text(element, 5, 1), "hi   ");
}

TEST(RenderToText, RendersMultipleLinesSeparatedByCarriageReturnNewline) {
  // ftxui::Screen::ToString() joins rows with "\r\n" (not "\n") because the
  // CR is needed to reset the terminal cursor's column on a real terminal.
  auto element = ftxui::vbox({ftxui::text("a"), ftxui::text("b")});
  EXPECT_EQ(render_to_text(element, 1, 2), "a\r\nb");
}

}  // namespace
}  // namespace terminal_ui_kit::test_support
