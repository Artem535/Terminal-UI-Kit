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

TEST(SyntaxHighlighter, ReportsLinkedGrammarAvailability) {
  EXPECT_TRUE(SyntaxHighlighter::supports_language("python"));
  EXPECT_TRUE(SyntaxHighlighter::supports_language("cpp"));
  EXPECT_FALSE(SyntaxHighlighter::supports_language("unknown"));
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
      "def greet(name: str): return \"hi\" + \"x\\n\" + 1.5 # note", "python", theme);
  const TextStyle* python_keyword = style_for_text(python, "def");
  const TextStyle* python_function = style_for_text(python, "greet");
  const TextStyle* python_string = style_for_text(python, "\"hi\"");
  const TextStyle* python_number = style_for_text(python, "1.5");
  ASSERT_NE(python_keyword, nullptr);
  ASSERT_NE(python_function, nullptr);
  ASSERT_NE(python_string, nullptr);
  ASSERT_NE(python_number, nullptr);
  EXPECT_EQ(*python_keyword, syntax.keyword);
  EXPECT_EQ(*python_function, syntax.function);
  EXPECT_EQ(*python_string, syntax.string);
  EXPECT_EQ(*python_number, syntax.number);

  const StyledText rust = SyntaxHighlighter::highlight(
      "fn greet(name: &str) { println!(\"hi\"); let escaped = \"x\\n\"; }", "rust", theme);
  const TextStyle* rust_keyword = style_for_text(rust, "fn");
  const TextStyle* rust_function = style_for_text(rust, "greet");
  const TextStyle* rust_macro = style_for_text(rust, "println");
  const TextStyle* rust_string = style_for_text(rust, "\"hi\"");
  const TextStyle* rust_escape = style_for_text(rust, "\\n");
  ASSERT_NE(rust_keyword, nullptr);
  ASSERT_NE(rust_function, nullptr);
  ASSERT_NE(rust_macro, nullptr);
  ASSERT_NE(rust_string, nullptr);
  ASSERT_NE(rust_escape, nullptr);
  EXPECT_EQ(*rust_keyword, syntax.keyword);
  EXPECT_EQ(*rust_function, syntax.function);
  EXPECT_EQ(*rust_macro, syntax.macro);
  EXPECT_EQ(*rust_string, syntax.string);
  EXPECT_EQ(*rust_escape, syntax.string);
}

TEST(SyntaxHighlighter, CoversCurrentCppNamespaceAndBooleanNodes) {
  const Theme& theme = default_dark_theme();
  const SyntaxTheme syntax = default_dark_syntax_theme(theme);
  const StyledText result =
      SyntaxHighlighter::highlight("namespace demo { bool enabled = true; }", "cpp", theme);

  const TextStyle* namespace_style = style_for_text(result, "demo");
  const TextStyle* boolean = style_for_text(result, "true");
  ASSERT_NE(namespace_style, nullptr);
  ASSERT_NE(boolean, nullptr);
  EXPECT_EQ(*namespace_style, syntax.namespace_style);
  EXPECT_EQ(*boolean, syntax.constant);
}

TEST(SyntaxHighlighter, CoversCurrentPythonFStringAndBooleanNodes) {
  const Theme& theme = default_dark_theme();
  const SyntaxTheme syntax = default_dark_syntax_theme(theme);
  const StyledText result =
      SyntaxHighlighter::highlight("message = f\"ready={True}\"", "python", theme);

  // The current Python grammar exposes an f-string as a string prefix/body
  // plus interpolation nodes, so the string capture is intentionally split.
  const TextStyle* string = style_for_text(result, "f\"ready=");
  const TextStyle* boolean = style_for_text(result, "True");
  ASSERT_NE(string, nullptr);
  ASSERT_NE(boolean, nullptr);
  EXPECT_EQ(*string, syntax.string);
  EXPECT_EQ(*boolean, syntax.constant);
}

TEST(SyntaxHighlighter, CoversCurrentJavascriptFunctionExpressionAndConstants) {
  const Theme& theme = default_dark_theme();
  const SyntaxTheme syntax = default_dark_syntax_theme(theme);
  const StyledText result = SyntaxHighlighter::highlight(
      "const fn = function named() { return this.value ?? null; };", "javascript", theme);

  const TextStyle* function = style_for_text(result, "named");
  const TextStyle* this_value = style_for_text(result, "this");
  const TextStyle* null_value = style_for_text(result, "null");
  ASSERT_NE(function, nullptr);
  ASSERT_NE(this_value, nullptr);
  ASSERT_NE(null_value, nullptr);
  EXPECT_EQ(*function, syntax.function);
  EXPECT_EQ(*this_value, syntax.variable);
  EXPECT_EQ(*null_value, syntax.constant);
}

TEST(SyntaxHighlighter, CoversRustLifetimesAndMacros) {
  const Theme& theme = default_dark_theme();
  const SyntaxTheme syntax = default_dark_syntax_theme(theme);
  const StyledText result = SyntaxHighlighter::highlight(
      "fn borrow<'a>(value: &'a str) { println!(\"{value}\"); }", "rust", theme);

  const TextStyle* macro = style_for_text(result, "println");
  // `lifetime` contains an identifier child; the overlap normalizer keeps
  // the most specific identifier spans, so assert source coverage instead of
  // requiring a synthetic combined span for `'a`.
  EXPECT_NE(flatten(result).find("'a"), std::string::npos);
  ASSERT_NE(macro, nullptr);
  EXPECT_EQ(*macro, syntax.macro);
}

TEST(SyntaxHighlighter, CoversCAndBashSemanticNodes) {
  const Theme& theme = default_dark_theme();
  const SyntaxTheme syntax = default_dark_syntax_theme(theme);

  const StyledText c =
      SyntaxHighlighter::highlight("#define LIMIT 10\nint main() { return LIMIT; }", "c", theme);
  // The preprocessor definition capture overlaps its child identifier and
  // is normalized into a `#define ` prefix plus the macro name.
  const TextStyle* c_macro = style_for_text(c, "#define ");
  const TextStyle* c_function = style_for_text(c, "main");
  ASSERT_NE(c_macro, nullptr);
  ASSERT_NE(c_function, nullptr);
  EXPECT_EQ(*c_macro, syntax.macro);
  EXPECT_EQ(*c_function, syntax.function);

  const StyledText bash =
      SyntaxHighlighter::highlight("echo \"hello\" # comment\ncount=42", "bash", theme);
  const TextStyle* command = style_for_text(bash, "echo");
  const TextStyle* comment = style_for_text(bash, "# comment");
  const TextStyle* number = style_for_text(bash, "42");
  ASSERT_NE(command, nullptr);
  ASSERT_NE(comment, nullptr);
  ASSERT_NE(number, nullptr);
  EXPECT_EQ(*command, syntax.function);
  EXPECT_EQ(*comment, syntax.comment);
  EXPECT_EQ(*number, syntax.number);
}

TEST(SyntaxHighlighter, CoversMarkdownYamlAndDiffBlockNodes) {
  const Theme& theme = default_dark_theme();
  const SyntaxTheme syntax = default_dark_syntax_theme(theme);

  if (!SyntaxHighlighter::supports_language("markdown") ||
      !SyntaxHighlighter::supports_language("yaml") ||
      !SyntaxHighlighter::supports_language("diff")) {
    GTEST_SKIP() << "optional Tree-sitter grammar is not linked";
  }

  const StyledText markdown = SyntaxHighlighter::highlight("# Title\n\n- item", "markdown", theme);
  const TextStyle* heading = style_for_text(markdown, "#");
  ASSERT_NE(heading, nullptr);
  EXPECT_EQ(*heading, syntax.keyword);

  const StyledText yaml = SyntaxHighlighter::highlight("name: value\nenabled: true", "yaml", theme);
  const TextStyle* key = style_for_text(yaml, "name");
  ASSERT_NE(key, nullptr);
  EXPECT_EQ(*key, syntax.property);

  const StyledText diff = SyntaxHighlighter::highlight("@@ -1 +1 @@\n+added", "diff", theme);
  const TextStyle* addition = style_for_text(diff, "+added");
  ASSERT_NE(addition, nullptr);
  EXPECT_EQ(*addition, syntax.string);
}

}  // namespace
}  // namespace terminal_ui_kit
