#include "terminal_ui_kit/core/text_position.h"

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TEST(TextPosition, EqualPositionsCompareEqual) {
  EXPECT_EQ((TextPosition{2, 5}), (TextPosition{2, 5}));
  EXPECT_FALSE((TextPosition{2, 5}) != (TextPosition{2, 5}));
}

TEST(TextPosition, DifferentPositionsCompareUnequal) {
  EXPECT_NE((TextPosition{2, 5}), (TextPosition{2, 6}));
  EXPECT_NE((TextPosition{2, 5}), (TextPosition{3, 5}));
}

TEST(TextPosition, OrdersByLineThenColumn) {
  EXPECT_LT((TextPosition{1, 9}), (TextPosition{2, 0}));
  EXPECT_LT((TextPosition{2, 0}), (TextPosition{2, 1}));
  EXPECT_FALSE((TextPosition{2, 1}) < (TextPosition{2, 1}));
}

}  // namespace
}  // namespace terminal_ui_kit
