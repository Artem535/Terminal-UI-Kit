#include "terminal_ui_kit/components/code_view.h"

#include <string>

#include <ftxui/screen/screen.hpp>

#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

// Extracts plain characters row by row, ignoring style/color metadata, so
// tests can assert on the visible text independent of ANSI styling.
std::string row_text(const ftxui::Screen& screen, int row) {
  std::string out;
  for (int x = 0; x < screen.dimx(); ++x) {
    out += screen.PixelAt(x, row).character;
  }
  return out;
}

CodeViewOptions LineNumbers() {
  CodeViewOptions opts;
  opts.show_line_numbers = true;
  return opts;
}

TEST(CodeView, ShortNumberRightAligned) {
  auto screen = test_support::render_to_screen(CodeView("aaa", LineNumbers()), 40, 1);
  EXPECT_EQ(row_text(screen, 0).substr(0, 6), "   1 a");
}

TEST(CodeView, WrappedLinesProduceSequentialLineNumbers) {
  // A single long line wraps into multiple display lines, each numbered.
  std::string text(80 * 3, 'x');
  auto screen = test_support::render_to_screen(CodeView(text, LineNumbers()), 120, 3);
  EXPECT_NE(row_text(screen, 0).find("   1 "), std::string::npos);
  EXPECT_NE(row_text(screen, 1).find("   2 "), std::string::npos);
  EXPECT_NE(row_text(screen, 2).find("   3 "), std::string::npos);
}

TEST(CodeView, LineNumberWiderThanGutterRendersFully) {
  // 10002 wrapped lines; line 10000 is exactly gutter width 4 and 10001/10002
  // exceed it. Both must render fully without truncation or underflow.
  std::string text(80 * 10002, 'x');
  auto screen = test_support::render_to_screen(CodeView(text, LineNumbers()), 120, 10002);
  EXPECT_NE(row_text(screen, 998).find(" 999 "), std::string::npos);
  EXPECT_NE(row_text(screen, 9998).find("9999 "), std::string::npos);
  EXPECT_NE(row_text(screen, 9999).find("10000 "), std::string::npos);
  EXPECT_NE(row_text(screen, 10000).find("10001 "), std::string::npos);
  EXPECT_NE(row_text(screen, 10001).find("10002 "), std::string::npos);
}

}  // namespace
}  // namespace terminal_ui_kit
