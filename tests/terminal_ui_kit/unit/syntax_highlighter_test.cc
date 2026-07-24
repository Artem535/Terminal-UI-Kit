#include "terminal_ui_kit/syntax/syntax_highlighter.h"

#include <string>
#include <string_view>

#include "terminal_ui_kit/core/styled_text.h"
#include "terminal_ui_kit/syntax/syntax_theme.h"
#include "terminal_ui_kit/theme/theme.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

std::string flatten(const StyledText& styled_text) {
  std::string result;
  for (const TextSpan& span : styled_text.spans()) {
    result += span.text;
  }
  return result;
}

const TextStyle* style_for_text(const StyledText& styled_text, std::string_view text) {
  for (const TextSpan& span : styled_text.spans()) {
    if (span.text == text) {
      return &span.style;
    }
  }
  return nullptr;
}

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

TEST(SyntaxHighlighter, PythonPreservesSourceTextWithoutDuplicateCaptures) {
  constexpr std::string_view code = "def fibonacci(n): return n";

  StyledText result = SyntaxHighlighter::highlight(code, "python", default_dark_theme());

  EXPECT_EQ(flatten(result), code);
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

TEST(SyntaxHighlighter, CppCapturesUseDistinctSyntaxRoles) {
  const Theme& theme = default_dark_theme();
  const SyntaxTheme syntax = default_dark_syntax_theme(theme);
  const std::string code =
      "namespace demo { int add(int value) { // note\n"
      "  return value + 42;\n"
      "}\nconst char* message = \"ok\"; }";

  const StyledText result = SyntaxHighlighter::highlight(code, "cpp", theme);
  const TextStyle* keyword = style_for_text(result, "return");
  const TextStyle* type = style_for_text(result, "int");
  const TextStyle* function = style_for_text(result, "add");
  const TextStyle* string = style_for_text(result, "\"ok\"");
  const TextStyle* number = style_for_text(result, "42");
  const TextStyle* comment = style_for_text(result, "// note");
  const TextStyle* namespace_style = style_for_text(result, "demo");
  const TextStyle* operator_style = style_for_text(result, "+");

  ASSERT_NE(keyword, nullptr);
  ASSERT_NE(type, nullptr);
  ASSERT_NE(function, nullptr);
  ASSERT_NE(string, nullptr);
  ASSERT_NE(number, nullptr);
  ASSERT_NE(comment, nullptr);
  ASSERT_NE(namespace_style, nullptr);
  ASSERT_NE(operator_style, nullptr);

  EXPECT_EQ(*keyword, syntax.keyword);
  EXPECT_EQ(*type, syntax.type);
  EXPECT_EQ(*function, syntax.function);
  EXPECT_EQ(*string, syntax.string);
  EXPECT_EQ(*number, syntax.number);
  EXPECT_EQ(*comment, syntax.comment);
  EXPECT_EQ(*namespace_style, syntax.namespace_style);
  EXPECT_EQ(*operator_style, syntax.operator_style);
  EXPECT_FALSE(keyword->background.has_value());
  EXPECT_FALSE(type->background.has_value());
  EXPECT_FALSE(function->background.has_value());
  EXPECT_NE(keyword->foreground, type->foreground);
  EXPECT_NE(type->foreground, function->foreground);
}

TEST(SyntaxHighlighter, PythonAndRustUseSharedSemanticPalette) {
  const Theme& theme = default_dark_theme();
  const SyntaxTheme syntax = default_dark_syntax_theme(theme);

  const StyledText python = SyntaxHighlighter::highlight(
      "def greet(name: str): return \"hi\" + name 42 # note", "python", theme);
  const TextStyle* python_keyword = style_for_text(python, "def");
  const TextStyle* python_function = style_for_text(python, "greet");
  const TextStyle* python_string = style_for_text(python, "\"hi\"");
  ASSERT_NE(python_keyword, nullptr);
  ASSERT_NE(python_function, nullptr);
  ASSERT_NE(python_string, nullptr);
  EXPECT_EQ(*python_keyword, syntax.keyword);
  EXPECT_EQ(*python_function, syntax.function);
  EXPECT_EQ(*python_string, syntax.string);

  const StyledText rust =
      SyntaxHighlighter::highlight("fn greet(name: &str) { println!(\"hi\"); }", "rust", theme);
  const TextStyle* rust_keyword = style_for_text(rust, "fn");
  const TextStyle* rust_function = style_for_text(rust, "greet");
  const TextStyle* rust_macro = style_for_text(rust, "println");
  const TextStyle* rust_string = style_for_text(rust, "\"hi\"");
  ASSERT_NE(rust_keyword, nullptr);
  ASSERT_NE(rust_function, nullptr);
  ASSERT_NE(rust_macro, nullptr);
  ASSERT_NE(rust_string, nullptr);
  EXPECT_EQ(*rust_keyword, syntax.keyword);
  EXPECT_EQ(*rust_function, syntax.function);
  EXPECT_EQ(*rust_macro, syntax.macro);
  EXPECT_EQ(*rust_string, syntax.string);
}

}  // namespace
}  // namespace terminal_ui_kit
