#include "terminal_ui_kit/core/line_number.h"

#include <cstddef>
#include <string>

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TEST(FormatLineNumber, WidthLargerThanDigitsRightAligns) {
  EXPECT_EQ(format_line_number(1, 4), "   1");
  EXPECT_EQ(format_line_number(9, 4), "   9");
  EXPECT_EQ(format_line_number(99, 4), "  99");
}

TEST(FormatLineNumber, WidthEqualToDigitsAddsNoPadding) {
  EXPECT_EQ(format_line_number(9999, 4), "9999");
  EXPECT_EQ(format_line_number(10000, 5), "10000");
}

TEST(FormatLineNumber, WidthSmallerThanDigitsRendersFully) {
  EXPECT_EQ(format_line_number(10000, 4), "10000");
  EXPECT_EQ(format_line_number(99999, 4), "99999");
  EXPECT_EQ(format_line_number(100000, 4), "100000");
}

TEST(FormatLineNumber, ZeroWidthRendersFully) {
  EXPECT_EQ(format_line_number(1, 0), "1");
  EXPECT_EQ(format_line_number(100000, 0), "100000");
}

TEST(FormatLineNumber, LargeValidLineNumber) {
  const std::size_t large = 1234567890123ULL;
  EXPECT_EQ(format_line_number(large, 4), std::to_string(large));
  EXPECT_EQ(format_line_number(large, 30).substr(0, 30 - std::to_string(large).size()),
            std::string(30 - std::to_string(large).size(), ' '));
}

}  // namespace
}  // namespace terminal_ui_kit
