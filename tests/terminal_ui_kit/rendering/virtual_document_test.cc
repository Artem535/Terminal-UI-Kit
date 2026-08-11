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

}  // namespace

TEST(VirtualDocument, ShortLineNumbersRightAligned) {
  StreamingDocument doc;
  doc.append("a\nb\nc");
  doc.finish();
  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.show_line_numbers = true;
  opts.follow = false;
  VirtualDocument view(opts);
  std::string text = test_support::render_to_text(view.component()->Render(), 40, 3);
  EXPECT_NE(text.find("   1"), std::string::npos);
  EXPECT_NE(text.find("   2"), std::string::npos);
}
TEST(VirtualDocument, LineNumbersWiderThanGutterRenderFully) {
  StreamingDocument doc;
  std::string content;
  for (int i = 1; i <= 100003; ++i) content += "x\n";
  content.pop_back();  // drop trailing newline so the last line is line 100003
  doc.append(content);
  doc.finish();
  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.show_line_numbers = true;
  opts.follow = true;
  VirtualDocument view(opts);
  // Gutter width is 5; numbers with 6+ digits exceed it and must render
  // fully without underflowing. Follow mode scrolls to the bottom. The old
  // implementation underflows and throws std::bad_alloc here.
  std::string text = test_support::render_to_text(view.component()->Render(), 40, 5);
  EXPECT_NE(text.find("100003"), std::string::npos);
}

}  // namespace terminal_ui_kit
