#include "terminal_ui_kit/core/line_number.h"

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TEST(format_line_number, SingleDigitRightAlignedToWidth) {
  EXPECT_EQ(format_line_number(1, 4), "   1");
  EXPECT_EQ(format_line_number(9, 4), "   9");
}

TEST(format_line_number, TwoDigitsRightAlignedToWidth) {
  EXPECT_EQ(format_line_number(99, 3), " 99");
  EXPECT_EQ(format_line_number(99, 4), "  99");
}

TEST(format_line_number, NumberWiderThanWidthIsNotTruncatedOrPadded) {
  // Regression: the old `width - digits.size()` arithmetic underflows when the
  // digit count exceeds the width, producing enormous padding.
  EXPECT_EQ(format_line_number(9999, 3), "9999");
  EXPECT_EQ(format_line_number(10000, 4), "10000");
  EXPECT_EQ(format_line_number(99999, 5), "99999");
  EXPECT_EQ(format_line_number(100000, 4), "100000");
  EXPECT_EQ(format_line_number(1000000, 2), "1000000");
  EXPECT_EQ(format_line_number(123456789012345678ULL, 4), "123456789012345678");
}

TEST(format_line_number, WidthEqualToDigitCountRendersWithoutPadding) {
  EXPECT_EQ(format_line_number(9999, 4), "9999");
  EXPECT_EQ(format_line_number(10000, 5), "10000");
}

TEST(format_line_number, WidthLargerThanDigitCountAddsLeadingSpaces) {
  EXPECT_EQ(format_line_number(1, 6), "     1");
  EXPECT_EQ(format_line_number(9999, 10), "      9999");
}

TEST(format_line_number, ZeroWidthIsSafe) {
  // A gutter width of zero must not underflow; the bare number is returned.
  EXPECT_EQ(format_line_number(1, 0), "1");
  EXPECT_EQ(format_line_number(9999, 0), "9999");
  EXPECT_EQ(format_line_number(100000, 0), "100000");
}

TEST(format_line_number, AbsurdWidthIsClamped) {
  // A very large configured width must not allocate an enormous padding
  // buffer; padding is capped at the helper's sane maximum (256).
  EXPECT_EQ(format_line_number(1, 1000000000u).size(), 256u);
  EXPECT_EQ(format_line_number(100000, 1000000000u).size(), 256u);
  EXPECT_EQ(format_line_number(1, 256u).size(), 256u);
}

TEST(format_line_number, ZeroNumberFormatsAsZero) {
  EXPECT_EQ(format_line_number(0, 4), "   0");
  EXPECT_EQ(format_line_number(0, 0), "0");
}

}  // namespace
}  // namespace terminal_ui_kit
