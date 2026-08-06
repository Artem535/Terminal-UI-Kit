#include "terminal_ui_kit/core/line_number.h"

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TEST(FormatLineNumber, SingleDigitRightAlignedToWidth) {
  EXPECT_EQ(FormatLineNumber(1, 4), "   1");
  EXPECT_EQ(FormatLineNumber(9, 4), "   9");
}

TEST(FormatLineNumber, TwoDigitsRightAlignedToWidth) {
  EXPECT_EQ(FormatLineNumber(99, 3), " 99");
  EXPECT_EQ(FormatLineNumber(99, 4), "  99");
}

TEST(FormatLineNumber, NumberWiderThanWidthIsNotTruncatedOrPadded) {
  // Regression: the old `width - digits.size()` arithmetic underflows when the
  // digit count exceeds the width, producing enormous padding.
  EXPECT_EQ(FormatLineNumber(9999, 3), "9999");
  EXPECT_EQ(FormatLineNumber(10000, 4), "10000");
  EXPECT_EQ(FormatLineNumber(99999, 5), "99999");
  EXPECT_EQ(FormatLineNumber(100000, 4), "100000");
  EXPECT_EQ(FormatLineNumber(1000000, 2), "1000000");
  EXPECT_EQ(FormatLineNumber(123456789012345678ULL, 4), "123456789012345678");
}

TEST(FormatLineNumber, WidthEqualToDigitCountRendersWithoutPadding) {
  EXPECT_EQ(FormatLineNumber(9999, 4), "9999");
  EXPECT_EQ(FormatLineNumber(10000, 5), "10000");
}

TEST(FormatLineNumber, WidthLargerThanDigitCountAddsLeadingSpaces) {
  EXPECT_EQ(FormatLineNumber(1, 6), "     1");
  EXPECT_EQ(FormatLineNumber(9999, 10), "      9999");
}

TEST(FormatLineNumber, ZeroWidthIsSafe) {
  // A gutter width of zero must not underflow; the bare number is returned.
  EXPECT_EQ(FormatLineNumber(1, 0), "1");
  EXPECT_EQ(FormatLineNumber(9999, 0), "9999");
  EXPECT_EQ(FormatLineNumber(100000, 0), "100000");
}

TEST(FormatLineNumber, ZeroNumberFormatsAsZero) {
  EXPECT_EQ(FormatLineNumber(0, 4), "   0");
  EXPECT_EQ(FormatLineNumber(0, 0), "0");
}

}  // namespace
}  // namespace terminal_ui_kit
