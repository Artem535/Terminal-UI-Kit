#include "terminal_ui_kit/theme/theme.h"

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TEST(Theme, DefaultConstructedRolesAreDefaultTextStyle) {
  Theme theme;
  EXPECT_EQ(theme.primary, TextStyle{});
  EXPECT_EQ(theme.error, TextStyle{});
}

TEST(Theme, EqualityComparesAllRoles) {
  Theme a;
  Theme b;
  EXPECT_EQ(a, b);

  b.accent.bold = true;
  EXPECT_NE(a, b);
}

TEST(Theme, DefaultDarkThemeIsStable) {
  EXPECT_EQ(default_dark_theme(), default_dark_theme());
}

TEST(Theme, DefaultLightThemeIsStable) {
  EXPECT_EQ(default_light_theme(), default_light_theme());
}

TEST(Theme, DarkAndLightThemesDiffer) {
  EXPECT_NE(default_dark_theme(), default_light_theme());
}

TEST(Theme, DarkThemeMarksNonColorAttributes) {
  const Theme& theme = default_dark_theme();
  EXPECT_TRUE(theme.error.bold);
  EXPECT_TRUE(theme.focused.bold);
  EXPECT_TRUE(theme.muted.dim);
}

TEST(Theme, LightThemeMarksNonColorAttributes) {
  const Theme& theme = default_light_theme();
  EXPECT_TRUE(theme.error.bold);
  EXPECT_TRUE(theme.focused.bold);
  EXPECT_TRUE(theme.muted.dim);
}

TEST(Theme, DarkThemeSetsRepresentativeColors) {
  const Theme& theme = default_dark_theme();
  ASSERT_TRUE(theme.primary.foreground.has_value());
  EXPECT_EQ(*theme.primary.foreground, (Color{0xe6, 0xed, 0xf3}));
  ASSERT_TRUE(theme.error.foreground.has_value());
  EXPECT_EQ(*theme.error.foreground, (Color{0xf8, 0x51, 0x49}));
  ASSERT_TRUE(theme.code.background.has_value());
  EXPECT_EQ(*theme.code.background, (Color{0x16, 0x1b, 0x22}));
}

TEST(Theme, LightThemeSetsRepresentativeColors) {
  const Theme& theme = default_light_theme();
  ASSERT_TRUE(theme.primary.foreground.has_value());
  EXPECT_EQ(*theme.primary.foreground, (Color{0x1f, 0x23, 0x28}));
  ASSERT_TRUE(theme.error.foreground.has_value());
  EXPECT_EQ(*theme.error.foreground, (Color{0xd1, 0x24, 0x2f}));
  ASSERT_TRUE(theme.code.background.has_value());
  EXPECT_EQ(*theme.code.background, (Color{0xf6, 0xf8, 0xfa}));
}

TEST(Theme, WithoutColorStripsForegroundAndBackgroundFromEveryRole) {
  const Theme stripped = without_color(default_dark_theme());

  const TextStyle* roles[] = {
      &stripped.primary,  &stripped.secondary, &stripped.muted,
      &stripped.success,  &stripped.warning,   &stripped.error,
      &stripped.accent,   &stripped.code,      &stripped.addition,
      &stripped.deletion, &stripped.border,    &stripped.selected,
      &stripped.focused,
  };

  for (const TextStyle* style : roles) {
    EXPECT_FALSE(style->foreground.has_value());
    EXPECT_FALSE(style->background.has_value());
  }
}

TEST(Theme, WithoutColorPreservesNonColorAttributes) {
  const Theme stripped = without_color(default_dark_theme());

  EXPECT_TRUE(stripped.error.bold);
  EXPECT_TRUE(stripped.focused.bold);
  EXPECT_TRUE(stripped.muted.dim);
}

}  // namespace
}  // namespace terminal_ui_kit
