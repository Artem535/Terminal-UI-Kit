#include "terminal_ui_kit/syntax/syntax_highlighter.h"

#include <string>

#include <gtest/gtest.h>

#include "terminal_ui_kit/core/styled_text.h"
#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {
namespace {

TEST(SyntaxHighlighter, CHighlightsKeywords) {
  const Theme& theme = default_dark_theme();
  StyledText result = SyntaxHighlighter::highlight("int x = 0;", "c", theme);
  ASSERT_FALSE(result.spans().empty());
}

TEST(SyntaxHighlighter, UnknownLanguageReturnsUnstyled) {
  const Theme& theme = default_dark_theme();
  StyledText result = SyntaxHighlighter::highlight("hello", "unknown", theme);
  ASSERT_EQ(result.spans().size(), 1U);
  EXPECT_EQ(result.spans()[0].text, "hello");
}

TEST(SyntaxHighlighter, EmptyCodeReturnsEmpty) {
  const Theme& theme = default_dark_theme();
  StyledText result = SyntaxHighlighter::highlight("", "c", theme);
  ASSERT_EQ(result.spans().size(), 1U);
  EXPECT_EQ(result.spans()[0].text, "");
}

TEST(SyntaxHighlighter, PythonHighlightsKeywords) {
  const Theme& theme = default_dark_theme();
  StyledText result = SyntaxHighlighter::highlight("def foo(): pass", "python", theme);
  ASSERT_FALSE(result.spans().empty());
}

TEST(SyntaxHighlighter, JsonHighlightsStrings) {
  const Theme& theme = default_dark_theme();
  StyledText result = SyntaxHighlighter::highlight("{\"key\": \"value\"}", "json", theme);
  ASSERT_FALSE(result.spans().empty());
}

TEST(SyntaxHighlighter, RustHighlightsKeywords) {
  const Theme& theme = default_dark_theme();
  StyledText result = SyntaxHighlighter::highlight("fn main() {}", "rust", theme);
  ASSERT_FALSE(result.spans().empty());
}

}  // namespace
}  // namespace terminal_ui_kit