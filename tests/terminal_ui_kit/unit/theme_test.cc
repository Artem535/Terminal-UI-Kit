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

}  // namespace
}  // namespace terminal_ui_kit
