#include "terminal_ui_kit/document/ansi_parser.h"

#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TEST(AnsiParser, PlainTextAndCoalescing) {
  const StyledText parsed = parse_ansi("hello\x1b[31mred\x1b[31m!\x1b[0m");
  ASSERT_EQ(parsed.spans().size(), 2u);
  EXPECT_EQ(parsed.spans()[0].text, "hello");
  EXPECT_EQ(parsed.spans()[1].text, "red!");
  EXPECT_EQ(parsed.spans()[1].style.foreground, (std::optional<Color>(Color{205, 0, 0})));
}

TEST(AnsiParser, AttributesAndReset) {
  const StyledText parsed = parse_ansi("\x1b[1;2;4;9mstyled\x1b[0mplain");
  ASSERT_EQ(parsed.spans().size(), 2u);
  EXPECT_TRUE(parsed.spans()[0].style.bold);
  EXPECT_TRUE(parsed.spans()[0].style.dim);
  EXPECT_TRUE(parsed.spans()[0].style.underline);
  EXPECT_TRUE(parsed.spans()[0].style.strikethrough);
  EXPECT_EQ(parsed.spans()[1].style, TextStyle{});
}

TEST(AnsiParser, StandardBrightAndRgbColors) {
  const StyledText parsed = parse_ansi("\x1b[91;44mbright\x1b[38;2;1;2;3;48;2;4;5;6mrgb");
  ASSERT_EQ(parsed.spans().size(), 2u);
  EXPECT_EQ(parsed.spans()[0].style.foreground, (std::optional<Color>(Color{255, 0, 0})));
  EXPECT_EQ(parsed.spans()[0].style.background, (std::optional<Color>(Color{0, 0, 238})));
  EXPECT_EQ(parsed.spans()[1].style.foreground, (std::optional<Color>(Color{1, 2, 3})));
  EXPECT_EQ(parsed.spans()[1].style.background, (std::optional<Color>(Color{4, 5, 6})));
}

TEST(AnsiParser, IndexedColors) {
  const StyledText parsed = parse_ansi("\x1b[38;5;196;48;5;232mx");
  ASSERT_EQ(parsed.spans().size(), 1u);
  EXPECT_EQ(parsed.spans()[0].style.foreground, (std::optional<Color>(Color{255, 0, 0})));
  EXPECT_EQ(parsed.spans()[0].style.background, (std::optional<Color>(Color{8, 8, 8})));
}

TEST(AnsiParser, RemovesUnsupportedControlsAndEscapes) {
  const StyledText parsed = parse_ansi("a\x1b[2Jb\x1b]0;title\a c\x1b]8;;url\x1b\\d");
  std::string plain;
  for (const TextSpan& span : parsed.spans()) plain += span.text;
  EXPECT_EQ(plain, "ab cd");
  EXPECT_EQ(plain.find('\x1b'), std::string::npos);
}

}  // namespace
}  // namespace terminal_ui_kit
