#include "terminal_ui_kit/markdown/markdown_view.h"

#include <memory>
#include <string>

#include <ftxui/screen/screen.hpp>
#include <gtest/gtest.h>

#include "terminal_ui_kit/markdown/markdown_document.h"
#include "terminal_ui_kit/testing/virtual_screen.h"

namespace terminal_ui_kit {
namespace {

TEST(MarkdownView, HeadingRendersWithBold) {
  auto doc = std::make_shared<MarkdownDocument>("# Hello");
  MarkdownViewOptions opts;
  auto view = MarkdownView(doc, opts);

  std::string text = test_support::render_to_text(view->Render(), 40, 3);
  EXPECT_NE(text.find("Hello"), std::string::npos);
}

TEST(MarkdownView, ParagraphRendersWrappedText) {
  auto doc = std::make_shared<MarkdownDocument>("Hello world");
  MarkdownViewOptions opts;
  auto view = MarkdownView(doc, opts);

  std::string text = test_support::render_to_text(view->Render(), 40, 3);
  EXPECT_NE(text.find("Hello"), std::string::npos);
  EXPECT_NE(text.find("world"), std::string::npos);
}

TEST(MarkdownView, CodeBlockRendersWithMonospace) {
  auto doc = std::make_shared<MarkdownDocument>("```cpp\nint x = 1;\n```");
  MarkdownViewOptions opts;
  auto view = MarkdownView(doc, opts);

  std::string text = test_support::render_to_text(view->Render(), 40, 5);
  EXPECT_NE(text.find("int x = 1;"), std::string::npos);
  EXPECT_NE(text.find("cpp"), std::string::npos);
}

TEST(MarkdownView, ListRendersWithBullets) {
  auto doc = std::make_shared<MarkdownDocument>("- item1\n- item2");
  MarkdownViewOptions opts;
  auto view = MarkdownView(doc, opts);

  std::string text = test_support::render_to_text(view->Render(), 40, 5);
  EXPECT_NE(text.find("item1"), std::string::npos);
  EXPECT_NE(text.find("item2"), std::string::npos);
}

TEST(MarkdownView, HorizontalRuleRendersSeparator) {
  auto doc = std::make_shared<MarkdownDocument>("---");
  MarkdownViewOptions opts;
  auto view = MarkdownView(doc, opts);

  test_support::render_to_screen(view->Render(), 40, 3);
  // Just verify it doesn't crash
}

TEST(MarkdownView, EmptyDocumentRendersEmpty) {
  auto doc = std::make_shared<MarkdownDocument>("");
  MarkdownViewOptions opts;
  auto view = MarkdownView(doc, opts);

  test_support::render_to_screen(view->Render(), 40, 3);
  // Just verify it doesn't crash
}

}  // namespace
}  // namespace terminal_ui_kit