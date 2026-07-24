#include "terminal_ui_kit/syntax/syntax_theme.h"

#include "terminal_ui_kit/theme/theme.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TEST(SyntaxTheme, DarkPaletteUsesDistinctSemanticColors) {
  const SyntaxTheme syntax = default_dark_syntax_theme(default_dark_theme());

  EXPECT_NE(syntax.keyword.foreground, syntax.type.foreground);
  EXPECT_NE(syntax.type.foreground, syntax.function.foreground);
  EXPECT_NE(syntax.function.foreground, syntax.string.foreground);
  EXPECT_NE(syntax.string.foreground, syntax.number.foreground);
  EXPECT_NE(syntax.number.foreground, syntax.comment.foreground);
  EXPECT_NE(syntax.comment.foreground, syntax.property.foreground);
  EXPECT_NE(syntax.property.foreground, syntax.namespace_style.foreground);
  EXPECT_NE(syntax.namespace_style.foreground, syntax.macro.foreground);
}

TEST(SyntaxTheme, SyntaxRolesDoNotHaveBackgrounds) {
  const SyntaxTheme syntax = default_dark_syntax_theme(default_dark_theme());
  const TextStyle* roles[] = {
      &syntax.keyword,         &syntax.type,           &syntax.function,
      &syntax.variable,        &syntax.string,         &syntax.number,
      &syntax.comment,         &syntax.operator_style, &syntax.property,
      &syntax.namespace_style, &syntax.macro,          &syntax.constant,
  };

  for (const TextStyle* style : roles) {
    EXPECT_FALSE(style->background.has_value());
  }
}

TEST(SyntaxTheme, LightPaletteUsesDistinctSemanticColors) {
  const SyntaxTheme syntax = default_light_syntax_theme(default_light_theme());

  EXPECT_NE(syntax.keyword.foreground, syntax.type.foreground);
  EXPECT_NE(syntax.type.foreground, syntax.function.foreground);
  EXPECT_NE(syntax.function.foreground, syntax.string.foreground);
  EXPECT_NE(syntax.string.foreground, syntax.number.foreground);
}

TEST(SyntaxTheme, FactoriesDoNotMutateBaseTheme) {
  const Theme theme = default_dark_theme();
  const Theme original = theme;
  static_cast<void>(default_dark_syntax_theme(theme));
  EXPECT_EQ(theme, original);
}

}  // namespace
}  // namespace terminal_ui_kit
