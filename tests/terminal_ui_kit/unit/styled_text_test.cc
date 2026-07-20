#include "terminal_ui_kit/core/styled_text.h"

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TEST(StyledText, StartsEmpty) {
  StyledText text;
  EXPECT_TRUE(text.spans().empty());
}

TEST(StyledText, AppendPreservesOrderAndContent) {
  StyledText text;
  TextStyle bold_style;
  bold_style.bold = true;

  text.append(TextSpan{"Hello, ", TextStyle{}, std::nullopt});
  text.append(TextSpan{"world", bold_style, std::nullopt});

  ASSERT_EQ(text.spans().size(), 2u);
  EXPECT_EQ(text.spans()[0].text, "Hello, ");
  EXPECT_FALSE(text.spans()[0].style.bold);
  EXPECT_EQ(text.spans()[1].text, "world");
  EXPECT_TRUE(text.spans()[1].style.bold);
}

TEST(StyledText, PreservesHyperlink) {
  StyledText text;
  text.append(TextSpan{"click here", TextStyle{}, Hyperlink{"https://example.com"}});

  ASSERT_EQ(text.spans().size(), 1u);
  ASSERT_TRUE(text.spans()[0].hyperlink.has_value());
  EXPECT_EQ(text.spans()[0].hyperlink->url, "https://example.com");
}

}  // namespace
}  // namespace terminal_ui_kit
