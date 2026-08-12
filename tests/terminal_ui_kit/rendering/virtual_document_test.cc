#include "terminal_ui_kit/components/virtual_document.h"

#include <cstddef>
#include <string>
#include <utility>

#include <ftxui/component/event.hpp>
#include <ftxui/screen/screen.hpp>

#include "terminal_ui_kit/document/streaming_document.h"
#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

// FTXUI's Screen::ToString() interleaves ANSI SGR escape sequences with the
// visible glyphs; strip them (and FTXUI's \r line suffix) so assertions can be
// written against plain text.
std::string StripAnsi(const std::string& in) {
  std::string out;
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size();) {
    if (in[i] == '\x1b' && i + 1 < in.size() && in[i + 1] == '[') {
      std::size_t j = i + 2;
      while (j < in.size() && !(static_cast<unsigned char>(in[j]) >= 0x40 &&
                                static_cast<unsigned char>(in[j]) < 0x80))
        ++j;
      if (j < in.size()) ++j;
      i = j;
    } else if (in[i] != '\r') {
      out.push_back(in[i++]);
    } else {
      ++i;
    }
  }
  return out;
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

TEST(VirtualDocument, ShortLineNumberRightAlignedWithinGutter) {
  StreamingDocument doc;
  doc.append("hello");  // no trailing newline -> a single line
  doc.finish();

  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.show_line_numbers = true;
  opts.gutter_width = 5;  // default
  VirtualDocument view(opts);

  std::string text = StripAnsi(test_support::render_to_text(view.component()->Render(), 30, 3));
  // "1" right-aligned in a 5-wide gutter, then the trailing separator.
  EXPECT_NE(text.find("    1 hello"), std::string::npos);
}

TEST(VirtualDocument, NumberLongerThanGutterRendersFullyWithoutHugePadding) {
  StreamingDocument doc;
  std::string content;
  for (int i = 1; i <= 100; ++i) {
    content += "line " + std::to_string(i);
    if (i != 100) content += "\n";
  }
  doc.append(content);
  doc.finish();

  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.show_line_numbers = true;
  opts.gutter_width = 2;  // line 100 (3 digits) is wider than the gutter
  VirtualDocument view(opts);

  // follow=true scrolls to the bottom, so the visible window shows line 100.
  std::string text = StripAnsi(test_support::render_to_text(view.component()->Render(), 30, 6));
  EXPECT_NE(text.find("100"), std::string::npos);
  // No enormous padding run may be rendered (old code underflowed here).
  EXPECT_EQ(text.find(std::string(50, ' ')), std::string::npos);
}

TEST(VirtualDocument, GutterWidthZeroIsSafe) {
  StreamingDocument doc;
  doc.append("hello");
  doc.finish();

  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.show_line_numbers = true;
  opts.gutter_width = 0;
  VirtualDocument view(opts);

  std::string text = StripAnsi(test_support::render_to_text(view.component()->Render(), 30, 3));
  EXPECT_NE(text.find("1 hello"), std::string::npos);
}

TEST(VirtualDocument, GutterWidthEqualToDigitsHasNoPadding) {
  StreamingDocument doc;
  doc.append("hello");
  doc.finish();

  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.show_line_numbers = true;
  opts.gutter_width = 1;  // "1" is exactly one digit wide
  VirtualDocument view(opts);

  std::string text = StripAnsi(test_support::render_to_text(view.component()->Render(), 30, 3));
  EXPECT_NE(text.find("1 hello"), std::string::npos);
}

TEST(VirtualDocument, GutterWidthLargerThanDigits) {
  StreamingDocument doc;
  doc.append("hello");
  doc.finish();

  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.show_line_numbers = true;
  opts.gutter_width = 3;
  VirtualDocument view(opts);

  std::string text = StripAnsi(test_support::render_to_text(view.component()->Render(), 30, 3));
  EXPECT_NE(text.find("  1 hello"), std::string::npos);
}

TEST(VirtualDocument, WrappedContinuationLinesIndentByGutterWidth) {
  StreamingDocument doc;
  doc.append(std::string(250, 'A'));  // wraps into multiple display lines
  doc.finish();

  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.show_line_numbers = true;
  opts.gutter_width = 5;
  VirtualDocument view(opts);  // follow defaults to true -> scrolls to bottom

  // The bottom-most display line is a continuation sub-line, indented by
  // gutter_width + 2 = 7 spaces (mirroring the pre-fix fixed gutter).
  std::string text = StripAnsi(test_support::render_to_text(view.component()->Render(), 40, 10));
  EXPECT_NE(text.find("       A"), std::string::npos);
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

}  // namespace
}  // namespace terminal_ui_kit