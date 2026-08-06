#include "terminal_ui_kit/components/virtual_document.h"

#include <string>
#include <utility>

#include <ftxui/component/event.hpp>
#include <ftxui/screen/screen.hpp>

#include "terminal_ui_kit/document/streaming_document.h"
#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

// The line-number gutter is coloured, so Screen::ToString() interleaves ANSI
// escapes around the number. Strip them before matching visible glyphs.
std::string strip_ansi(const std::string& in) {
  std::string out;
  out.reserve(in.size());
  std::size_t i = 0;
  while (i < in.size()) {
    if (in[i] == '\x1b' && i + 1 < in.size() && in[i + 1] == '[') {
      i += 2;
      while (i < in.size() && !(in[i] >= 0x40 && in[i] <= 0x7e)) {
        ++i;
      }
      if (i < in.size()) {
        ++i;  // consume the CSI final byte
      }
      continue;
    }
    out.push_back(in[i]);
    ++i;
  }
  return out;
}

// Builds `count` logical lines ("l1\nl2\n...\nlN") with no trailing newline,
// so the bottom-most logical line is a real content line rather than the
// empty line introduced by a trailing newline.
std::string build_lines(int count) {
  std::string content;
  content.reserve(static_cast<std::size_t>(count) * 6);
  for (int i = 1; i <= count; ++i) {
    content += "l" + std::to_string(i);
    if (i < count) {
      content += '\n';
    }
  }
  return content;
}

TEST(VirtualDocument, EmptyDocumentRendersEmpty) {
  StreamingDocument doc;

  VirtualDocumentOptions opts;
  opts.document = &doc;
  VirtualDocument view(opts);

  ftxui::Screen screen = test_support::render_to_screen(view.component()->Render(), 20, 3);
  std::string text = screen.ToString();
  // Nothing meaningful rendered — just whitespace/newlines
  // FTXUI's ToString uses \r\n line endings; just check no meaningful text
  EXPECT_TRUE(text.find("hello") == std::string::npos);
  EXPECT_TRUE(text.find("world") == std::string::npos);
}

TEST(VirtualDocument, ShortLinesDisplayCorrectly) {
  StreamingDocument doc;
  doc.append("hello\nworld");
  doc.finish();

  VirtualDocumentOptions opts;
  opts.document = &doc;
  VirtualDocument view(opts);

  test_support::render_to_screen(view.component()->Render(), 20, 3);
  std::string text = test_support::render_to_text(view.component()->Render(), 20, 3);
  EXPECT_NE(text.find("hello"), std::string::npos);
  EXPECT_NE(text.find("world"), std::string::npos);
}

TEST(VirtualDocument, ShowLineNumbersOnFirstSubLine) {
  StreamingDocument doc;
  doc.append("hello");
  doc.finish();

  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.show_line_numbers = true;
  VirtualDocument view(opts);

  test_support::render_to_screen(view.component()->Render(), 20, 3);
  std::string text = test_support::render_to_text(view.component()->Render(), 20, 3);
  EXPECT_NE(text.find("1"), std::string::npos);
  EXPECT_NE(text.find("hello"), std::string::npos);
}

TEST(VirtualDocument, ArrowUpDisablesFollow) {
  StreamingDocument doc;
  doc.append("line1\nline2\nline3");
  doc.finish();

  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.follow = true;
  VirtualDocument view(opts);

  view.component()->OnEvent(ftxui::Event::ArrowUp);
  EXPECT_FALSE(view.follow());
}

TEST(VirtualDocument, EndReenablesFollow) {
  StreamingDocument doc;
  doc.append("line1");
  doc.finish();

  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.follow = true;
  VirtualDocument view(opts);

  view.component()->OnEvent(ftxui::Event::ArrowUp);
  EXPECT_FALSE(view.follow());

  view.component()->OnEvent(ftxui::Event::End);
  EXPECT_TRUE(view.follow());
}

TEST(VirtualDocument, SetFollowReenablesFollowing) {
  StreamingDocument doc;
  doc.append("line1");
  doc.finish();

  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.follow = true;
  VirtualDocument view(opts);

  view.set_follow(false);
  EXPECT_FALSE(view.follow());

  view.set_follow(true);
  EXPECT_TRUE(view.follow());
}

TEST(VirtualDocument, FollowAutoScrollsOnNewData) {
  StreamingDocument doc;
  doc.append("line1");
  doc.finish();

  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.follow = true;
  VirtualDocument view(opts);

  view.component()->Render();

  doc.append("line2");
  doc.finish();

  std::string text = test_support::render_to_text(view.component()->Render(), 20, 2);
  EXPECT_NE(text.find("line2"), std::string::npos);
}

TEST(VirtualDocument, WrappedLinesRenderMultipleDisplayLines) {
  StreamingDocument doc;
  doc.append(std::string(60, 'A'));
  doc.finish();

  VirtualDocumentOptions opts;
  opts.document = &doc;
  VirtualDocument view(opts);

  test_support::render_to_screen(view.component()->Render(), 20, 5);
  std::string text = test_support::render_to_text(view.component()->Render(), 20, 5);
  EXPECT_NE(text.find("AAA"), std::string::npos);
}

TEST(VirtualDocument, VThenArrowThenYSelectsAndYanks) {
  StreamingDocument doc;
  doc.append("line1\nline2\nline3");
  doc.finish();

  std::string copied;
  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.on_copy = [&copied](std::string text) { copied = std::move(text); };
  VirtualDocument view(opts);

  // Render to initialize VirtualList selection at line0
  view.component()->Render();

  // v to start selection at line0
  view.component()->OnEvent(ftxui::Event::Character("v"));

  // ArrowDown to extend selection to line1
  view.component()->OnEvent(ftxui::Event::ArrowDown);
  view.component()->Render();

  // ArrowDown to extend to line2
  view.component()->OnEvent(ftxui::Event::ArrowDown);
  view.component()->Render();

  // y to yank (copy)
  view.component()->OnEvent(ftxui::Event::Character("y"));

  EXPECT_NE(copied.find("line1"), std::string::npos);
  EXPECT_NE(copied.find("line2"), std::string::npos);
}

TEST(VirtualDocument, WideLineNumberWiderThanGutterRendersFully) {
  StreamingDocument doc;
  doc.append(build_lines(12));
  doc.finish();

  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.show_line_numbers = true;
  // Gutter width 1 is narrower than the 2-digit number "12".
  opts.line_number_width = 1;
  VirtualDocument view(opts);

  std::string text = strip_ansi(test_support::render_to_text(view.component()->Render(), 20, 6));
  // The bottom-most logical line "12" must render fully, never truncated and
  // without the enormous padding produced by the old underflowing arithmetic.
  EXPECT_NE(text.find("12 l12"), std::string::npos);
  EXPECT_EQ(text.find(std::string(30, ' ')), std::string::npos);
}

TEST(VirtualDocument, LineNumberWidthEqualToDigitCountHasNoPadding) {
  StreamingDocument doc;
  doc.append(build_lines(12));
  doc.finish();

  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.show_line_numbers = true;
  opts.line_number_width = 2;  // exactly the digit count of "12"
  VirtualDocument view(opts);

  std::string text = strip_ansi(test_support::render_to_text(view.component()->Render(), 20, 6));
  EXPECT_NE(text.find("12 l12"), std::string::npos);
  EXPECT_EQ(text.find(std::string(30, ' ')), std::string::npos);
}

TEST(VirtualDocument, LineNumberWidthLargerThanDigitCountRightAligns) {
  StreamingDocument doc;
  doc.append(build_lines(12));
  doc.finish();

  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.show_line_numbers = true;
  opts.line_number_width = 4;  // wider than the 2-digit number "12"
  VirtualDocument view(opts);

  std::string text = strip_ansi(test_support::render_to_text(view.component()->Render(), 20, 6));
  // Right-aligned in a 4-column gutter.
  EXPECT_NE(text.find("  12 l12"), std::string::npos);
}

TEST(VirtualDocument, LargeValidLineNumberWiderThanGutterRendersFully) {
  StreamingDocument doc;
  doc.append(build_lines(1000));
  doc.finish();

  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.show_line_numbers = true;
  opts.line_number_width = 2;  // narrower than the 4-digit number "1000"
  VirtualDocument view(opts);

  std::string text = strip_ansi(test_support::render_to_text(view.component()->Render(), 20, 6));
  EXPECT_NE(text.find("1000 l1000"), std::string::npos);
  EXPECT_EQ(text.find(std::string(30, ' ')), std::string::npos);
}

TEST(VirtualDocument, DefaultGutterWidthUnderflowRegressionAt100000) {
  // Old implementation hard-coded gutter width 5 and computed
  // `std::string(5 - digits, ' ')`, which underflows for the 6-digit line
  // number "100000" - the old code would allocate an astronomically large
  // padding string here. The new implementation renders the number unpadded.
  StreamingDocument doc;
  doc.append(build_lines(100000));
  doc.finish();

  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.show_line_numbers = true;  // default line_number_width == 5
  VirtualDocument view(opts);

  std::string text = strip_ansi(test_support::render_to_text(view.component()->Render(), 20, 6));
  EXPECT_NE(text.find("100000 l100000"), std::string::npos);
  EXPECT_EQ(text.find(std::string(30, ' ')), std::string::npos);
}

TEST(VirtualDocument, ZeroLineNumberWidthIsSafe) {
  StreamingDocument doc;
  doc.append("hello");
  doc.finish();

  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.show_line_numbers = true;
  opts.line_number_width = 0;
  VirtualDocument view(opts);

  std::string text = strip_ansi(test_support::render_to_text(view.component()->Render(), 20, 3));
  EXPECT_NE(text.find("1 hello"), std::string::npos);
}

}  // namespace
}  // namespace terminal_ui_kit