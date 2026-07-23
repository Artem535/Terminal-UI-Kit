#include "terminal_ui_kit/markdown/markdown_document.h"

#include <string>

#include <cmark-gfm.h>
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TEST(MarkdownDocument, ParsesHeading) {
  MarkdownDocument doc("# Hello");
  ASSERT_NE(doc.root(), nullptr);
  cmark_node* first = cmark_node_first_child(doc.root());
  ASSERT_NE(first, nullptr);
  EXPECT_EQ(cmark_node_get_type(first), CMARK_NODE_HEADING);
  EXPECT_EQ(cmark_node_get_heading_level(first), 1);
}

TEST(MarkdownDocument, ParsesParagraph) {
  MarkdownDocument doc("Hello world");
  ASSERT_NE(doc.root(), nullptr);
  cmark_node* first = cmark_node_first_child(doc.root());
  ASSERT_NE(first, nullptr);
  EXPECT_EQ(cmark_node_get_type(first), CMARK_NODE_PARAGRAPH);
}

TEST(MarkdownDocument, ParsesBold) {
  MarkdownDocument doc("**bold**");
  ASSERT_NE(doc.root(), nullptr);
  cmark_node* para = cmark_node_first_child(doc.root());
  ASSERT_NE(para, nullptr);
  cmark_node* strong = cmark_node_first_child(para);
  ASSERT_NE(strong, nullptr);
  EXPECT_EQ(cmark_node_get_type(strong), CMARK_NODE_STRONG);
}

TEST(MarkdownDocument, ParsesEmphasis) {
  MarkdownDocument doc("*italic*");
  ASSERT_NE(doc.root(), nullptr);
  cmark_node* para = cmark_node_first_child(doc.root());
  ASSERT_NE(para, nullptr);
  cmark_node* emph = cmark_node_first_child(para);
  ASSERT_NE(emph, nullptr);
  EXPECT_EQ(cmark_node_get_type(emph), CMARK_NODE_EMPH);
}

TEST(MarkdownDocument, ParsesInlineCode) {
  MarkdownDocument doc("`code`");
  ASSERT_NE(doc.root(), nullptr);
  cmark_node* para = cmark_node_first_child(doc.root());
  ASSERT_NE(para, nullptr);
  cmark_node* code = cmark_node_first_child(para);
  ASSERT_NE(code, nullptr);
  EXPECT_EQ(cmark_node_get_type(code), CMARK_NODE_CODE);
}

TEST(MarkdownDocument, ParsesFencedCodeBlock) {
  MarkdownDocument doc("```cpp\nint x = 1;\n```");
  ASSERT_NE(doc.root(), nullptr);
  cmark_node* first = cmark_node_first_child(doc.root());
  ASSERT_NE(first, nullptr);
  EXPECT_EQ(cmark_node_get_type(first), CMARK_NODE_CODE_BLOCK);
  std::string info = cmark_node_get_fence_info(first);
  EXPECT_EQ(info, "cpp");
}

TEST(MarkdownDocument, ParsesUnorderedList) {
  MarkdownDocument doc("- item1\n- item2");
  ASSERT_NE(doc.root(), nullptr);
  cmark_node* list = cmark_node_first_child(doc.root());
  ASSERT_NE(list, nullptr);
  EXPECT_EQ(cmark_node_get_type(list), CMARK_NODE_LIST);
  EXPECT_EQ(cmark_node_get_list_type(list), CMARK_BULLET_LIST);
}

TEST(MarkdownDocument, ParsesOrderedList) {
  MarkdownDocument doc("1. first\n2. second");
  ASSERT_NE(doc.root(), nullptr);
  cmark_node* list = cmark_node_first_child(doc.root());
  ASSERT_NE(list, nullptr);
  EXPECT_EQ(cmark_node_get_type(list), CMARK_NODE_LIST);
  EXPECT_EQ(cmark_node_get_list_type(list), CMARK_ORDERED_LIST);
}

TEST(MarkdownDocument, ParsesHorizontalRule) {
  MarkdownDocument doc("---");
  ASSERT_NE(doc.root(), nullptr);
  cmark_node* first = cmark_node_first_child(doc.root());
  ASSERT_NE(first, nullptr);
  EXPECT_EQ(cmark_node_get_type(first), CMARK_NODE_THEMATIC_BREAK);
}

TEST(MarkdownDocument, ParsesLink) {
  MarkdownDocument doc("[text](https://example.com)");
  ASSERT_NE(doc.root(), nullptr);
  cmark_node* para = cmark_node_first_child(doc.root());
  ASSERT_NE(para, nullptr);
  cmark_node* link = cmark_node_first_child(para);
  ASSERT_NE(link, nullptr);
  EXPECT_EQ(cmark_node_get_type(link), CMARK_NODE_LINK);
  std::string url = cmark_node_get_url(link);
  EXPECT_EQ(url, "https://example.com");
}

TEST(MarkdownDocument, EmptyDocument) {
  MarkdownDocument doc("");
  ASSERT_NE(doc.root(), nullptr);
  EXPECT_EQ(cmark_node_first_child(doc.root()), nullptr);
}

}  // namespace
}  // namespace terminal_ui_kit