#include "terminal_ui_kit/core/text_range.h"

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TEST(TextRange, ContainsPositionWithinHalfOpenBounds) {
  TextRange range{TextPosition{1, 0}, TextPosition{3, 0}};
  EXPECT_TRUE(range.contains(TextPosition{1, 0}));   // start is inclusive
  EXPECT_TRUE(range.contains(TextPosition{2, 5}));
  EXPECT_FALSE(range.contains(TextPosition{3, 0}));  // end is exclusive
  EXPECT_FALSE(range.contains(TextPosition{0, 9}));
}

TEST(TextRange, IsEmptyWhenStartEqualsEnd) {
  TextRange range{TextPosition{1, 1}, TextPosition{1, 1}};
  EXPECT_TRUE(range.is_empty());
}

TEST(TextRange, IsNotEmptyWhenStartPrecedesEnd) {
  TextRange range{TextPosition{1, 0}, TextPosition{1, 1}};
  EXPECT_FALSE(range.is_empty());
}

}  // namespace
}  // namespace terminal_ui_kit
