#include "terminal_ui_kit/editor/multiline_editor.h"

#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/event.hpp>
#include <ftxui/screen/screen.hpp>

#include "terminal_ui_kit/editor/command_history.h"
#include "terminal_ui_kit/editor/editor_document.h"
#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

ftxui::Event SubmitEvent() {
  // Matches the default Ctrl+Enter submit binding (CSI-u).
  return ftxui::Event::Special("\x1B[13;5u");
}

TEST(MultilineEditor, RendersTextAndInvertedCaret) {
  MultilineEditor editor;
  editor.set_text("hello");
  auto component = editor.component();
  const ftxui::Screen screen = test_support::render_to_screen(component->Render(), 20, 3);
  EXPECT_EQ(screen.PixelAt(0, 0).character, "h");
  EXPECT_EQ(screen.PixelAt(4, 0).character, "o");
  // The caret sits at the cursor (col 0) and is inverted.
  EXPECT_TRUE(screen.PixelAt(0, 0).inverted);
  EXPECT_FALSE(screen.PixelAt(1, 0).inverted);
}

TEST(MultilineEditor, EmptyDocumentIsSafeAndRenders) {
  MultilineEditor editor;
  editor.component()->OnEvent(ftxui::Event::Backspace);
  editor.component()->OnEvent(ftxui::Event::ArrowLeft);
  editor.component()->OnEvent(ftxui::Event::ArrowUp);
  auto component = editor.component();
  test_support::render_to_text(component->Render(), 20, 3);
  EXPECT_EQ(editor.document().text(), "");
}

TEST(MultilineEditor, CharacterTypingUpdatesDocument) {
  MultilineEditor editor;
  editor.component()->OnEvent(ftxui::Event::Character("a"));
  editor.component()->OnEvent(ftxui::Event::Character("b"));
  EXPECT_EQ(editor.document().text(), "ab");
}

TEST(MultilineEditor, BackspaceAndDelete) {
  MultilineEditor editor;
  editor.set_text("abcd");
  auto c = editor.component();
  for (int i = 0; i < 4; ++i) {
    c->OnEvent(ftxui::Event::ArrowRight);  // col 4
  }
  c->OnEvent(ftxui::Event::Backspace);  // removes 'd'
  c->OnEvent(ftxui::Event::ArrowLeft);  // col 2
  c->OnEvent(ftxui::Event::Delete);     // removes 'c'
  EXPECT_EQ(editor.document().text(), "ab");
}

TEST(MultilineEditor, ReturnInsertsNewline) {
  MultilineEditor editor;
  editor.component()->OnEvent(ftxui::Event::Character("x"));
  editor.component()->OnEvent(ftxui::Event::Return);
  editor.component()->OnEvent(ftxui::Event::Character("y"));
  EXPECT_EQ(editor.document().line_count(), 2U);
  EXPECT_EQ(editor.document().line(0), "x");
  EXPECT_EQ(editor.document().line(1), "y");
}

TEST(MultilineEditor, ArrowNavigationMovesCursor) {
  MultilineEditor editor;
  editor.set_text("abcd\nefgh");
  auto c = editor.component();
  c->OnEvent(ftxui::Event::ArrowRight);
  c->OnEvent(ftxui::Event::ArrowRight);
  c->OnEvent(ftxui::Event::ArrowDown);
  EXPECT_EQ(editor.document().cursor(), (TextPosition{1, 2}));
  c->OnEvent(ftxui::Event::ArrowLeft);
  c->OnEvent(ftxui::Event::ArrowUp);
  EXPECT_EQ(editor.document().cursor(), (TextPosition{0, 1}));
}

TEST(MultilineEditor, WordNavigationMovesByWords) {
  MultilineEditor editor;
  editor.set_text("alpha beta");
  auto c = editor.component();
  c->OnEvent(ftxui::Event::ArrowRightCtrl);  // word right
  EXPECT_EQ(editor.document().cursor().column, 5U);
}

TEST(MultilineEditor, SubmitInvokesCallbackWithBufferCopy) {
  std::string submitted;
  MultilineEditorOptions options;
  options.on_submit = [&](std::string value) { submitted = std::move(value); };
  MultilineEditor editor(std::move(options));
  editor.component()->OnEvent(ftxui::Event::Character("x"));
  editor.component()->OnEvent(ftxui::Event::Character("y"));
  editor.component()->OnEvent(SubmitEvent());
  EXPECT_EQ(submitted, "xy");
  // Submit must not corrupt the buffer.
  EXPECT_EQ(editor.document().text(), "xy");
}

TEST(MultilineEditor, SubmitWithMultilineValue) {
  std::string submitted;
  MultilineEditorOptions options;
  options.on_submit = [&](std::string value) { submitted = std::move(value); };
  MultilineEditor editor(std::move(options));
  editor.component()->OnEvent(ftxui::Event::Character("a"));
  editor.component()->OnEvent(ftxui::Event::Return);
  editor.component()->OnEvent(ftxui::Event::Character("b"));
  editor.component()->OnEvent(SubmitEvent());
  EXPECT_EQ(submitted, "a\nb");
}

TEST(MultilineEditor, BracketedPasteIsInsertedAsOneBlock) {
  MultilineEditor editor;
  // Simulate a bracketed paste delivered as a single Character event with
  // embedded newlines.
  editor.component()->OnEvent(ftxui::Event::Character("line1\nline2\nline3"));
  EXPECT_EQ(editor.document().line_count(), 3U);
  EXPECT_EQ(editor.document().line(0), "line1");
  EXPECT_EQ(editor.document().line(1), "line2");
  EXPECT_EQ(editor.document().line(2), "line3");
  EXPECT_EQ(editor.document().text(), "line1\nline2\nline3");
}

TEST(MultilineEditor, HistoryRecallAndEditExitsMode) {
  CommandHistory history;
  history.add("echo one");
  history.add("echo two");
  MultilineEditorOptions options;
  options.history = &history;
  MultilineEditor editor(std::move(options));
  auto c = editor.component();

  // history_previous (Ctrl+Up) recalls the newest entry.
  c->OnEvent(ftxui::Event::ArrowUpCtrl);
  EXPECT_EQ(editor.document().text(), "echo two");
  // Another press recalls an older entry.
  c->OnEvent(ftxui::Event::ArrowUpCtrl);
  EXPECT_EQ(editor.document().text(), "echo one");
  // Editing exits history mode.
  c->OnEvent(ftxui::Event::Character("x"));
  EXPECT_FALSE(history.navigating());
  EXPECT_EQ(editor.document().text(), "echo onex");
}

TEST(MultilineEditor, HistoryNextRestoresDraft) {
  CommandHistory history;
  history.add("one");
  history.add("two");
  MultilineEditorOptions options;
  options.history = &history;
  MultilineEditor editor(std::move(options));
  auto c = editor.component();

  editor.component()->OnEvent(ftxui::Event::Character("z"));  // draft "z"
  c->OnEvent(ftxui::Event::ArrowUpCtrl);                      // recall "two"
  EXPECT_EQ(editor.document().text(), "two");
  c->OnEvent(ftxui::Event::ArrowDownCtrl);  // at newest -> restore draft
  EXPECT_EQ(editor.document().text(), "z");
  EXPECT_FALSE(history.navigating());
}

TEST(MultilineEditor, HistoryDisabledIgnoresBindings) {
  CommandHistory history;
  history.add("one");
  MultilineEditorOptions options;
  options.history = &history;
  options.history_enabled = false;
  MultilineEditor editor(std::move(options));
  auto c = editor.component();
  // Ctrl+Up should not recall when history is disabled.
  EXPECT_FALSE(c->OnEvent(ftxui::Event::ArrowUpCtrl));
  EXPECT_EQ(editor.document().text(), "");
}

TEST(MultilineEditor, ResizeUpdatesViewport) {
  MultilineEditor editor;
  auto c = editor.component();
  test_support::render_to_screen(c->Render(), 30, 6);
  EXPECT_EQ(editor.document().viewport_width(), 30U);
  EXPECT_EQ(editor.document().viewport_height(), 6U);
  test_support::render_to_screen(c->Render(), 20, 3);
  EXPECT_EQ(editor.document().viewport_width(), 20U);
  EXPECT_EQ(editor.document().viewport_height(), 3U);
}

TEST(MultilineEditor, ViewportScrollsWithCursor) {
  std::vector<std::string> lines;
  for (int i = 0; i < 100; ++i) {
    lines.push_back("line " + std::to_string(i));
  }
  MultilineEditor editor;
  editor.set_text("line\n0\n");  // dummy, replaced below
  editor.document().set_lines(std::move(lines));
  editor.document().set_viewport_size(20, 5);
  editor.document().set_cursor({50, 0});

  const ftxui::Screen screen = test_support::render_to_screen(editor.component()->Render(), 20, 5);
  EXPECT_GE(editor.document().scroll_top(), 46U);
  // The first visible row (index 0) shows line 46 and the last visible row
  // (index 4) shows the cursor line 50. "line 46" -> col 5 is '4';
  // "line 50" -> col 5 is '5', col 6 is '0'.
  EXPECT_EQ(screen.PixelAt(5, 0).character, "4");
  EXPECT_EQ(screen.PixelAt(5, 4).character, "5");
  EXPECT_EQ(screen.PixelAt(6, 4).character, "0");
}

TEST(MultilineEditor, CursorStaysVisibleAfterNavigation) {
  std::vector<std::string> lines;
  for (int i = 0; i < 100; ++i) {
    lines.push_back("row");
  }
  MultilineEditor editor;
  editor.document().set_lines(std::move(lines));
  editor.document().set_viewport_size(20, 5);
  editor.document().set_cursor({90, 0});
  // After a render the viewport tracks the cursor.
  test_support::render_to_screen(editor.component()->Render(), 20, 5);
  const std::size_t line = editor.document().cursor().line;
  EXPECT_GE(line, editor.document().scroll_top());
  EXPECT_LT(line, editor.document().scroll_top() + editor.document().viewport_height());
}

TEST(MultilineEditor, ConfiguredKeyBindingOverridesDefault) {
  MultilineEditorOptions options;
  options.key_bindings.move_left = ftxui::Event::Character("h");  // vim-style
  MultilineEditor editor(std::move(options));
  auto c = editor.component();
  editor.set_text("abc");
  for (int i = 0; i < 3; ++i) {
    c->OnEvent(ftxui::Event::ArrowRight);
  }
  c->OnEvent(ftxui::Event::Character("h"));
  EXPECT_EQ(editor.document().cursor().column, 2U);
}

}  // namespace
}  // namespace terminal_ui_kit
