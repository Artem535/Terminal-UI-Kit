#include "terminal_ui_kit/core/text_wrap.h"

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TEST(WrapPlainText, EmptyInputYieldsOneEmptyLine) {
  EXPECT_EQ(wrap_plain_text("", 10), (std::vector<std::string>{""}));
}

TEST(WrapPlainText, TextShorterThanWidthFitsOnOneLine) {
  EXPECT_EQ(wrap_plain_text("hello", 10), (std::vector<std::string>{"hello"}));
}

TEST(WrapPlainText, BreaksOnSpaceAndDropsIt) {
  EXPECT_EQ(wrap_plain_text("hello world", 5),
            (std::vector<std::string>{"hello", "world"}));
}

TEST(WrapPlainText, HardBreaksWordLongerThanWidth) {
  EXPECT_EQ(wrap_plain_text("abcdefgh", 3),
            (std::vector<std::string>{"abc", "def", "gh"}));
}

TEST(WrapPlainText, KeepsMultiByteUtf8CodepointsIntact) {
  // 6 Cyrillic codepoints, 2 bytes each in UTF-8; wrapping at 3 codepoints
  // must split between codepoints, never inside one.
  EXPECT_EQ(wrap_plain_text("\xd0\xbf\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82", 3),
            (std::vector<std::string>{"\xd0\xbf\xd1\x80\xd0\xb8", "\xd0\xb2\xd0\xb5\xd1\x82"}));
}

}  // namespace
}  // namespace terminal_ui_kit
