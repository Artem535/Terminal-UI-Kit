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

int find_text(const ftxui::Screen& screen, std::string_view needle, int* row) {
  for (int y = 0; y < screen.dimy(); ++y) {
    for (int x = 0; x + static_cast<int>(needle.size()) <= screen.dimx(); ++x) {
      bool matches = true;
      for (std::size_t i = 0; i < needle.size(); ++i) {
        if (screen.PixelAt(x + static_cast<int>(i), y).character != std::string(1, needle[i])) {
          matches = false;
          break;
        }
      }
      if (matches) {
        *row = y;
        return x;
      }
    }
  }
  return -1;
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

TEST(MarkdownView, SyntaxTypesAndNamespacesHaveNoBackground) {
  auto doc = std::make_shared<MarkdownDocument>("```cpp\nstd::vector<int> values;\n```");
  MarkdownViewOptions opts;
  auto view = MarkdownView(doc, opts);

  ftxui::Screen screen = test_support::render_to_screen(view->Render(), 60, 6);
  int namespace_row = -1;
  const int namespace_column = find_text(screen, "std", &namespace_row);
  ASSERT_GE(namespace_column, 0);
  ASSERT_GE(namespace_row, 0);

  int type_row = -1;
  const int type_column = find_text(screen, "vector", &type_row);
  ASSERT_GE(type_column, 0);
  ASSERT_EQ(type_row, namespace_row);

  const ftxui::Pixel& namespace_pixel = screen.PixelAt(namespace_column, namespace_row);
  const ftxui::Pixel& type_pixel = screen.PixelAt(type_column, type_row);
  EXPECT_NE(namespace_pixel.foreground_color, ftxui::Color::Default);
  EXPECT_NE(type_pixel.foreground_color, ftxui::Color::Default);
  EXPECT_EQ(namespace_pixel.background_color, ftxui::Color::Default);
  EXPECT_EQ(type_pixel.background_color, ftxui::Color::Default);
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
