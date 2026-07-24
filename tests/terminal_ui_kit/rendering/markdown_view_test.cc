#include "terminal_ui_kit/markdown/markdown_view.h"

#include <memory>
#include <string>
#include <string_view>

#include <ftxui/screen/screen.hpp>

#include "terminal_ui_kit/markdown/markdown_document.h"
#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

std::string strip_ansi(std::string_view text) {
  std::string result;
  for (std::size_t i = 0; i < text.size();) {
    if (text[i] != '\x1b') {
      result += text[i++];
      continue;
    }

    ++i;
    if (i >= text.size()) {
      break;
    }
    if (text[i] == '[') {
      ++i;
      while (i < text.size() && !(text[i] >= '@' && text[i] <= '~')) {
        ++i;
      }
      if (i < text.size()) {
        ++i;
      }
    } else if (text[i] == ']') {
      ++i;
      while (i < text.size()) {
        if (text[i] == '\a') {
          ++i;
          break;
        }
        if (text[i] == '\x1b' && i + 1 < text.size() && text[i + 1] == '\\') {
          i += 2;
          break;
        }
        ++i;
      }
    } else {
      ++i;
    }
  }
  return result;
}

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
  std::string plain_text = strip_ansi(text);
  EXPECT_NE(plain_text.find("int x = 1;"), std::string::npos);
  EXPECT_NE(plain_text.find("cpp"), std::string::npos);
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
