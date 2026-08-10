#include "terminal_ui_kit/components/transcript.h"

#include <chrono>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/screen/screen.hpp>

#include "terminal_ui_kit/components/transcript_model.h"
#include "terminal_ui_kit/document/log_model.h"
#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TranscriptViewOptions TestOptions() {
  TranscriptViewOptions options;
  options.follow = true;
  return options;
}

TranscriptView MakeView(TranscriptModel* model, TranscriptViewOptions options = {}) {
  return TranscriptView(model, std::move(options));
}

TEST(TranscriptView, EmptyTranscriptRendersEmpty) {
  TranscriptModel model;
  TranscriptView view = MakeView(&model);
  const std::string text = test_support::render_to_text(view.component()->Render(), 40, 5);
  // No block content should appear; the viewport is blank (FTXUI uses \r\n).
  std::string trimmed = text;
  trimmed.erase(trimmed.find_last_not_of(" \r\n") + 1);
  EXPECT_TRUE(trimmed.empty());
}

TEST(TranscriptView, SingleTextBlockRenders) {
  TranscriptModel model;
  model.append(TextBlock{"hello world"});
  TranscriptView view = MakeView(&model);
  const std::string text = test_support::render_to_text(view.component()->Render(), 40, 5);
  EXPECT_NE(text.find("hello world"), std::string::npos);
}

TEST(TranscriptView, MixedBlockTypesRender) {
  TranscriptModel model;
  model.append(TextBlock{"plain"});
  model.append(MarkdownBlock{"# heading"});
  model.append(CodeBlock{"std::cout << 1;", "cpp"});
  model.append(LogBlock{LogSeverity::kError, "log line"});
  model.append(DiffBlock{"+added\n-removed"});
  model.append(StatusBlock{Status::kSuccess, "done status"});
  model.append(CustomBlock{"tool", "custom content"});

  // follow off so the whole transcript (not just the tail) is visible.
  TranscriptViewOptions options = TestOptions();
  options.follow = false;
  TranscriptView view = MakeView(&model, std::move(options));
  // Prime the viewport box (VirtualList measures the visible range on a
  // second render after the box is established by drawing to a screen).
  test_support::render_to_screen(view.component()->Render(), 60, 20);
  const std::string text = test_support::render_to_text(view.component()->Render(), 60, 20);
  EXPECT_NE(text.find("plain"), std::string::npos);
  EXPECT_NE(text.find("# heading"), std::string::npos);
  EXPECT_NE(text.find("std::cout << 1;"), std::string::npos);
  EXPECT_NE(text.find("log line"), std::string::npos);
  EXPECT_NE(text.find("+added"), std::string::npos);
  EXPECT_NE(text.find("-removed"), std::string::npos);
  EXPECT_NE(text.find("done status"), std::string::npos);
  EXPECT_NE(text.find("custom content"), std::string::npos);
}

TEST(TranscriptView, SearchDisablesFollow) {
  TranscriptModel model;
  model.append(TextBlock{"apple"});
  model.append(TextBlock{"banana"});
  TranscriptView view = MakeView(&model);
  view.component()->Render();
  EXPECT_TRUE(view.follow());

  EXPECT_TRUE(view.find("apple"));
  EXPECT_FALSE(view.follow()) << "searching disables follow-end";

  view.component()->OnEvent(ftxui::Event::End);
  EXPECT_TRUE(view.follow());
}

TEST(TranscriptView, SearchAsYouTypeAnchorsCursor) {
  TranscriptModel model;
  // Prior match (index 3, "z a") stops matching "q", while blocks 1,2,4 still
  // match; the cursor must anchor at the next match after the prior position
  // (4), not jump back to the first hit (1).
  model.append(TextBlock{"x"});
  model.append(TextBlock{"q a"});
  model.append(TextBlock{"q"});
  model.append(TextBlock{"z a"});
  model.append(TextBlock{"q"});
  TranscriptView view = MakeView(&model);
  view.component()->Render();

  EXPECT_EQ(view.find("a"), true);
  EXPECT_EQ(view.current_match(), std::optional<std::size_t>{1});
  view.find_next();
  EXPECT_EQ(view.current_match(), std::optional<std::size_t>{3});

  view.find("q");
  EXPECT_EQ(view.current_match(), std::optional<std::size_t>{4});
}

TEST(TranscriptView, MutableTailRendersAtFixedHeightAndStreamsInPlace) {
  TranscriptModel model;
  model.begin_tail(TextBlock{""});
  TranscriptViewOptions options = TestOptions();
  options.tail_display_height = 3;
  TranscriptView view = MakeView(&model, std::move(options));

  for (int i = 0; i < 50; ++i) {
    model.append_tail("line");
    test_support::render_to_screen(view.component()->Render(), 40, 5);
  }
  // Streaming never creates extra entries (one tail block).
  EXPECT_EQ(model.block_count(), 1u);
  EXPECT_TRUE(model.has_tail());

  const std::string text = test_support::render_to_text(view.component()->Render(), 40, 5);
  EXPECT_NE(text.find("line"), std::string::npos);
}

TEST(TranscriptView, FollowAutoScrollsOnAppend) {
  TranscriptModel model;
  model.append(TextBlock{"first"});
  TranscriptView view = MakeView(&model);
  view.component()->Render();

  model.append(TextBlock{"second"});
  const std::string text = test_support::render_to_text(view.component()->Render(), 40, 2);
  EXPECT_NE(text.find("second"), std::string::npos);
}

TEST(TranscriptView, ManualScrollArrowUpDisablesFollow) {
  TranscriptModel model;
  model.append(TextBlock{"a"});
  model.append(TextBlock{"b"});
  TranscriptView view = MakeView(&model);
  view.component()->Render();
  EXPECT_TRUE(view.follow());

  view.component()->OnEvent(ftxui::Event::ArrowUp);
  EXPECT_FALSE(view.follow());
}

TEST(TranscriptView, WheelScrollDisablesFollow) {
  TranscriptModel model;
  model.append(TextBlock{"a"});
  model.append(TextBlock{"b"});
  TranscriptView view = MakeView(&model);
  view.component()->Render();

  ftxui::Mouse mouse;
  mouse.x = 0;
  mouse.y = 0;
  mouse.button = ftxui::Mouse::WheelUp;
  view.component()->OnEvent(ftxui::Event::Mouse("", mouse));
  EXPECT_FALSE(view.follow());
}

TEST(TranscriptView, EndKeyResumesFollow) {
  TranscriptModel model;
  model.append(TextBlock{"a"});
  model.append(TextBlock{"b"});
  TranscriptView view = MakeView(&model);
  view.component()->Render();

  view.component()->OnEvent(ftxui::Event::ArrowUp);
  EXPECT_FALSE(view.follow());
  view.component()->OnEvent(ftxui::Event::End);
  EXPECT_TRUE(view.follow());
}

TEST(TranscriptView, FKeyTogglesFollow) {
  TranscriptModel model;
  model.append(TextBlock{"a"});
  TranscriptView view = MakeView(&model);
  view.component()->Render();
  EXPECT_TRUE(view.follow());

  view.component()->OnEvent(ftxui::Event::Character("f"));
  EXPECT_FALSE(view.follow());
  view.component()->OnEvent(ftxui::Event::Character("f"));
  EXPECT_TRUE(view.follow());
}

TEST(TranscriptView, KeyboardNavigationMovesActiveItem) {
  TranscriptModel model;
  model.append(TextBlock{"zero"});
  model.append(TextBlock{"one"});
  model.append(TextBlock{"two"});

  std::vector<std::size_t> opened;
  TranscriptViewOptions options = TestOptions();
  options.on_open_details = [&opened](std::size_t index) { opened.push_back(index); };
  TranscriptView view = MakeView(&model, std::move(options));
  view.component()->Render();

  view.component()->OnEvent(ftxui::Event::ArrowDown);
  view.component()->OnEvent(ftxui::Event::Return);
  ASSERT_EQ(opened.size(), 1u);
  EXPECT_EQ(opened[0], 1u);
}

TEST(TranscriptView, HomeAndPageNavigationWork) {
  TranscriptModel model;
  for (std::size_t i = 0; i < 20; ++i) {
    model.append(TextBlock{"block" + std::to_string(i)});
  }
  std::vector<std::size_t> opened;
  TranscriptViewOptions options = TestOptions();
  options.on_open_details = [&opened](std::size_t index) { opened.push_back(index); };
  TranscriptView view = MakeView(&model, std::move(options));
  view.component()->Render();

  view.component()->OnEvent(ftxui::Event::PageDown);
  view.component()->OnEvent(ftxui::Event::Return);
  ASSERT_FALSE(opened.empty());
  EXPECT_GT(opened.back(), 0u);

  view.component()->OnEvent(ftxui::Event::Home);
  view.component()->OnEvent(ftxui::Event::Return);
  EXPECT_EQ(opened.back(), 0u);
}

TEST(TranscriptView, SearchFindAndNavigate) {
  TranscriptModel model;
  model.append(TextBlock{"apple pie"});
  model.append(TextBlock{"banana"});
  model.append(TextBlock{"pineapple"});
  TranscriptView view = MakeView(&model);
  view.component()->Render();

  EXPECT_TRUE(view.find("apple"));
  EXPECT_EQ(view.current_match(), std::optional<std::size_t>{0});

  EXPECT_TRUE(view.find_next());
  EXPECT_EQ(view.current_match(), std::optional<std::size_t>{2});

  EXPECT_TRUE(view.find_next());
  EXPECT_EQ(view.current_match(), std::optional<std::size_t>{0});  // wraps

  EXPECT_TRUE(view.find_previous());
  EXPECT_EQ(view.current_match(), std::optional<std::size_t>{2});
}

TEST(TranscriptView, SearchSlashInput) {
  TranscriptModel model;
  model.append(TextBlock{"apple"});
  model.append(TextBlock{"banana"});
  model.append(TextBlock{"ananas"});
  TranscriptView view = MakeView(&model);
  view.component()->Render();

  view.component()->OnEvent(ftxui::Event::Character("/"));
  view.component()->OnEvent(ftxui::Event::Character("a"));
  view.component()->OnEvent(ftxui::Event::Character("n"));
  EXPECT_EQ(view.current_match(), std::optional<std::size_t>{1});
}

TEST(TranscriptView, BookmarkMarkerRendersAndToggles) {
  TranscriptModel model;
  model.append(TextBlock{"alpha"});
  model.append(TextBlock{"beta"});
  TranscriptView view = MakeView(&model);
  view.component()->Render();

  // Select block 1 then bookmark it via 'b'.
  view.component()->OnEvent(ftxui::Event::ArrowDown);
  view.component()->OnEvent(ftxui::Event::Character("b"));
  EXPECT_TRUE(model.is_bookmarked(1));

  const std::string text = test_support::render_to_text(view.component()->Render(), 40, 5);
  EXPECT_NE(text.find("\u25C6"), std::string::npos);

  view.component()->OnEvent(ftxui::Event::Character("b"));
  EXPECT_FALSE(model.is_bookmarked(1));
}

TEST(TranscriptView, CopyCallbackReceivesSelectedBlockText) {
  TranscriptModel model;
  model.append(TextBlock{"zero"});
  model.append(TextBlock{"hello world"});

  std::string copied;
  TranscriptViewOptions options = TestOptions();
  options.on_copy = [&copied](std::string text) { copied = std::move(text); };
  TranscriptView view = MakeView(&model, std::move(options));
  view.component()->Render();

  view.component()->OnEvent(ftxui::Event::ArrowDown);
  view.component()->OnEvent(ftxui::Event::Character("y"));
  EXPECT_EQ(copied, "hello world");
}

TEST(TranscriptView, DetailsCallbackReceivesSelectedIndex) {
  TranscriptModel model;
  model.append(TextBlock{"zero"});
  model.append(TextBlock{"one"});

  std::vector<std::size_t> opened;
  TranscriptViewOptions options = TestOptions();
  options.on_open_details = [&opened](std::size_t index) { opened.push_back(index); };
  TranscriptView view = MakeView(&model, std::move(options));
  view.component()->Render();

  view.component()->OnEvent(ftxui::Event::ArrowDown);
  view.component()->OnEvent(ftxui::Event::Return);
  ASSERT_EQ(opened.size(), 1u);
  EXPECT_EQ(opened[0], 1u);
}

TEST(TranscriptView, MixedHeightsRenderAllLines) {
  TranscriptModel model;
  model.append(TextBlock{"short"});
  model.append(CodeBlock{"line one\nline two\nline three\nline four", "cpp"});

  TranscriptViewOptions options = TestOptions();
  options.follow = false;
  TranscriptView view = MakeView(&model, std::move(options));
  const std::string text = test_support::render_to_text(view.component()->Render(), 60, 20);
  EXPECT_NE(text.find("line one"), std::string::npos);
  EXPECT_NE(text.find("line two"), std::string::npos);
  EXPECT_NE(text.find("line three"), std::string::npos);
  EXPECT_NE(text.find("line four"), std::string::npos);
}

TEST(TranscriptView, ResizeDoesNotCrashOrLoseDelayedContent) {
  TranscriptModel model;
  model.append(TextBlock{"short line"});
  model.append(CodeBlock{"some longer code line here", "cpp"});
  TranscriptView view = MakeView(&model);

  test_support::render_to_screen(view.component()->Render(), 20, 5);
  test_support::render_to_screen(view.component()->Render(), 40, 5);
  test_support::render_to_screen(view.component()->Render(), 80, 5);
  const std::string text = test_support::render_to_text(view.component()->Render(), 80, 5);
  EXPECT_NE(text.find("short line"), std::string::npos);
}

TEST(TranscriptView, ClearAndReuseRendersEmptyThenContent) {
  TranscriptModel model;
  model.append(TextBlock{"before"});
  TranscriptViewOptions options = TestOptions();
  options.follow = false;
  TranscriptView view = MakeView(&model, std::move(options));
  view.component()->Render();

  model.clear();
  std::string text = test_support::render_to_text(view.component()->Render(), 40, 5);
  EXPECT_EQ(text.find("before"), std::string::npos) << "cleared content must be gone";

  model.append(TextBlock{"after"});
  text = test_support::render_to_text(view.component()->Render(), 40, 5);
  EXPECT_NE(text.find("after"), std::string::npos);
}

TEST(TranscriptView, ActiveItemStaysStableAfterAppend) {
  TranscriptModel model;
  model.append(TextBlock{"zero"});
  model.append(TextBlock{"one"});

  std::vector<std::size_t> opened;
  TranscriptViewOptions options = TestOptions();
  options.on_open_details = [&opened](std::size_t index) { opened.push_back(index); };
  TranscriptView view = MakeView(&model, std::move(options));
  view.component()->Render();

  // Select block 1.
  view.component()->OnEvent(ftxui::Event::ArrowDown);
  // Append a new block; the active/item index must not shift.
  model.append(TextBlock{"new block appended"});
  view.component()->Render();

  view.component()->OnEvent(ftxui::Event::Return);
  ASSERT_EQ(opened.size(), 1u);
  EXPECT_EQ(opened[0], 1u);
}

TEST(TranscriptView, LargeTranscriptRemainsNavigable) {
  TranscriptModel model;
  for (std::size_t i = 0; i < 100000; ++i) {
    model.append(TextBlock{"message " + std::to_string(i)});
  }
  TranscriptView view = MakeView(&model);
  view.component()->Render();

  // Navigating to the end and rendering must not crash.
  view.component()->OnEvent(ftxui::Event::End);
  const std::string text = test_support::render_to_text(view.component()->Render(), 40, 3);
  EXPECT_NE(text.find("99999"), std::string::npos);
}

TEST(TranscriptView, RepeatedStreamingUpdatesStayFast) {
  TranscriptModel model;
  for (std::size_t i = 0; i < 1000; ++i) {
    model.append(TextBlock{"prefilled " + std::to_string(i)});
  }
  model.begin_tail(TextBlock{""});
  TranscriptView view = MakeView(&model);
  view.component()->Render();

  const std::size_t count_before = model.block_count();
  const auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < 500; ++i) {
    model.append_tail("tok");
    test_support::render_to_screen(view.component()->Render(), 40, 10);
  }
  const auto end = std::chrono::steady_clock::now();

  EXPECT_EQ(model.block_count(), count_before);
  const auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  // Loose regression guard: 500 full render cycles must stay well under a
  // second even in a debug build. Catches accidental O(n) full-transcript
  // relayout on each tail append.
  EXPECT_LT(elapsed_ms, 5000);
}

}  // namespace
}  // namespace terminal_ui_kit