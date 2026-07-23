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

TEST(WrapPlainTextWithOffsets, ShortTextFitsOnOneLine) {
  auto result = wrap_plain_text_with_offsets("hello", 80);
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0].first, "hello");
  EXPECT_EQ(result[0].second, 0u);
}

TEST(WrapPlainTextWithOffsets, BreaksOnSpaceAndDropsIt) {
  auto result = wrap_plain_text_with_offsets("hello world", 5);
  ASSERT_EQ(result.size(), 2u);
  EXPECT_EQ(result[0].first, "hello");
  EXPECT_EQ(result[0].second, 0u);
  EXPECT_EQ(result[1].first, "world");
  EXPECT_EQ(result[1].second, 6u);
}

TEST(WrapPlainTextWithOffsets, HardBreaksWordLongerThanWidth) {
  auto result = wrap_plain_text_with_offsets("abcde", 2);
  ASSERT_EQ(result.size(), 3u);
  EXPECT_EQ(result[0].first, "ab");
  EXPECT_EQ(result[0].second, 0u);
  EXPECT_EQ(result[1].first, "cd");
  EXPECT_EQ(result[1].second, 2u);
  EXPECT_EQ(result[2].first, "e");
  EXPECT_EQ(result[2].second, 4u);
}

TEST(WrapPlainTextWithOffsets, EmptyInputYieldsOneEmptySegment) {
  auto result = wrap_plain_text_with_offsets("", 10);
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0].first, "");
  EXPECT_EQ(result[0].second, 0u);
}

TEST(WrapPlainTextWithOffsets, HandlesMultiByteUtf8) {
  // "café" — 4 codepoints, 'é' is 2 bytes (0xC3 0xA9)
  auto result = wrap_plain_text_with_offsets("caf\xc3\xa9", 3);
  ASSERT_EQ(result.size(), 2u);
  EXPECT_EQ(result[0].first, "caf");
  EXPECT_EQ(result[0].second, 0u);
  EXPECT_EQ(result[1].first, "\xc3\xa9");
  EXPECT_EQ(result[1].second, 3u);
}

}  // namespace
}  // namespace terminal_ui_kit
