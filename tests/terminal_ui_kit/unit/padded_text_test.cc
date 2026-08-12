#include "terminal_ui_kit/core/padded_text.h"

#include <string>

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TEST(PaddedText, ShorterThanWidthIsRightAligned) {
  EXPECT_EQ(pad_left_to_width("1", 4), "   1");
  EXPECT_EQ(pad_left_to_width("9", 4), "   9");
  EXPECT_EQ(pad_left_to_width("99", 4), "  99");
  EXPECT_EQ(pad_left_to_width("999", 4), " 999");
}

TEST(PaddedText, ExactlyEqualToWidthHasNoPadding) {
  EXPECT_EQ(pad_left_to_width("9999", 4), "9999");
  EXPECT_EQ(pad_left_to_width("10000", 5), "10000");
}

TEST(PaddedText, LongerThanWidthIsNotTruncatedAndHasNoPadding) {
  // These previously underflowed `4 - size()` / `5 - size()`.
  EXPECT_EQ(pad_left_to_width("10000", 4), "10000");
  EXPECT_EQ(pad_left_to_width("99999", 4), "99999");
  EXPECT_EQ(pad_left_to_width("100000", 5), "100000");
  EXPECT_EQ(pad_left_to_width("100000", 3), "100000");
}

TEST(PaddedText, LargeNumberIsReturnedInFull) {
  EXPECT_EQ(pad_left_to_width("12345678901234567890", 8), "12345678901234567890");
}

TEST(PaddedText, ZeroWidthHandlesAnyInputSafely) {
  EXPECT_EQ(pad_left_to_width("1", 0), "1");
  EXPECT_EQ(pad_left_to_width("100000", 0), "100000");
  EXPECT_EQ(pad_left_to_width("", 0), "");
}

TEST(PaddedText, EmptyTextInNonZeroWidthYieldsOnlySpaces) {
  EXPECT_EQ(pad_left_to_width("", 4), "    ");
}

}  // namespace
}  // namespace terminal_ui_kit
