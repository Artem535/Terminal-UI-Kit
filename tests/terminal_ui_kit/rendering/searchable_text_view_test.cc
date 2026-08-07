#include "terminal_ui_kit/components/searchable_text_view.h"

#include <string>
#include <utility>
#include <vector>

#include <ftxui/screen/screen.hpp>

#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

// Strip CSI escape sequences whose final byte is in 0x40..0x7e.
std::string StripAnsi(const std::string& in) {
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
        ++i;
      }
      continue;
    }
    out.push_back(in[i]);
    ++i;
  }
  return out;
}

std::string RenderText(SearchableTextView& view, int w, int h) {
  return StripAnsi(test_support::render_to_text(view.component()->Render(), w, h));
}

TEST(SearchableTextView, EmptyDocumentRendersEmpty) {
  SearchableTextView view;
  view.set_lines({});
  EXPECT_EQ(view.status(), SearchStatus::kEmptyQuery);
  std::string text = RenderText(view, 20, 3);
  EXPECT_TRUE(text.find("hello") == std::string::npos);
}

TEST(SearchableTextView, RendersDocumentContent) {
  SearchableTextView view;
  view.set_lines({"line one", "line two", "line three"});
  std::string text = RenderText(view, 30, 5);
  EXPECT_NE(text.find("line one"), std::string::npos);
}

TEST(SearchableTextView, Utf8ContentIsPreserved) {
  // UTF-8 document content must render unchanged (no corruption).
  SearchableTextView view;
  view.set_lines({"\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82"});
  std::string text = RenderText(view, 20, 3);
  EXPECT_NE(text.find("\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82"), std::string::npos);
}

TEST(SearchableTextView, OneMatchCounted) {
  SearchableTextView view;
  view.set_lines({"foo bar", "hello"});
  view.set_query("bar");
  EXPECT_EQ(view.status(), SearchStatus::kMatches);
  EXPECT_EQ(view.match_count(), 1u);
  ASSERT_TRUE(view.current_match_index().has_value());
  EXPECT_EQ(*view.current_match_index(), 0u);
}

TEST(SearchableTextView, MultipleMatches) {
  SearchableTextView view;
  view.set_lines({"foo foo foo", "no match"});
  view.set_query("foo");
  EXPECT_EQ(view.match_count(), 3u);
}

TEST(SearchableTextView, NoResultsState) {
  SearchableTextView view;
  view.set_lines({"foo", "bar"});
  view.set_query("zzz");
  EXPECT_EQ(view.status(), SearchStatus::kNoResults);
  EXPECT_EQ(view.match_count(), 0u);
  EXPECT_FALSE(view.current_match_index().has_value());
}

TEST(SearchableTextView, ClearNoResultsStateOnQueryChange) {
  SearchableTextView view;
  view.set_lines({"foo", "bar"});
  view.set_query("zzz");
  EXPECT_EQ(view.status(), SearchStatus::kNoResults);
  view.set_query("foo");
  EXPECT_EQ(view.status(), SearchStatus::kMatches);
  EXPECT_EQ(view.match_count(), 1u);
}

TEST(SearchableTextView, NextMatchWraps) {
  SearchableTextView view;
  view.set_lines({"alpha", "beta alpha", "gamma"});
  view.set_query("alpha");
  EXPECT_EQ(view.match_count(), 2u);
  EXPECT_EQ(*view.current_match_index(), 0u);
  view.next_match();
  EXPECT_EQ(*view.current_match_index(), 1u);
  view.next_match();  // wraps
  EXPECT_EQ(*view.current_match_index(), 0u);
}

TEST(SearchableTextView, PreviousMatchWraps) {
  SearchableTextView view;
  view.set_lines({"alpha", "beta alpha", "gamma"});
  view.set_query("alpha");
  view.previous_match();  // wraps to last
  EXPECT_EQ(*view.current_match_index(), 1u);
  view.previous_match();
  EXPECT_EQ(*view.current_match_index(), 0u);
}

TEST(SearchableTextView, ToggleCaseSensitivity) {
  SearchableTextView view;
  view.set_lines({"Foo foo"});
  view.set_query("foo");
  EXPECT_EQ(view.match_count(), 2u);  // case-insensitive default
  view.toggle_case_sensitive();
  EXPECT_TRUE(view.case_sensitive());
  EXPECT_EQ(view.match_count(), 1u);  // only "foo"
  view.toggle_case_sensitive();
  EXPECT_FALSE(view.case_sensitive());
  EXPECT_EQ(view.match_count(), 2u);
}

TEST(SearchableTextView, RegexModeAndInvalidRegex) {
  SearchableTextView view;
  view.set_lines({"foo", "boo", "bar"});
  view.toggle_regex();
  EXPECT_TRUE(view.use_regex());
  view.set_query("[fb]oo");
  EXPECT_EQ(view.status(), SearchStatus::kMatches);
  EXPECT_EQ(view.match_count(), 2u);

  view.set_query("(");  // invalid regex
  EXPECT_EQ(view.status(), SearchStatus::kInvalidRegex);
  EXPECT_EQ(view.match_count(), 0u);
}

TEST(SearchableTextView, ActiveMatchScrollsIntoView) {
  // Build a document tall enough that line 49 is far below the initial
  // viewport; navigating to its match must scroll it into view.
  SearchableTextView view;
  std::vector<std::string> lines;
  for (int i = 0; i < 60; ++i) {
    lines.push_back("line " + std::to_string(i) + " needle");
  }
  view.set_lines(lines);
  view.set_query("needle");
  view.previous_match();  // wraps to the last match (line 49)
  EXPECT_EQ(*view.current_match_index(), 59u);

  // Prime a render so the virtual list learns its full box, then assert.
  test_support::render_to_screen(view.component()->Render(), 30, 10);
  std::string text = RenderText(view, 30, 10);
  EXPECT_NE(text.find("line 59"), std::string::npos);
  // The first line should be scrolled out of view.
  EXPECT_TRUE(text.find("line 0") == std::string::npos);
}

TEST(SearchableTextView, MatchAtViewportBoundaryOnLastLine) {
  SearchableTextView view;
  std::vector<std::string> lines;
  for (int i = 0; i < 5; ++i) {
    lines.push_back("row " + std::to_string(i));
  }
  lines.push_back("target at end");
  view.set_lines(lines);
  view.set_query("target");
  view.previous_match();  // go to the only match (last line)
  test_support::render_to_screen(view.component()->Render(), 30, 10);
  std::string text = RenderText(view, 30, 10);
  EXPECT_NE(text.find("target at end"), std::string::npos);
}

TEST(SearchableTextView, RepeatedRenderWithoutQueryChangeIsStable) {
  SearchableTextViewOptions opts;
  opts.show_line_numbers = false;
  SearchableTextView view(std::move(opts));
  view.set_lines({"alpha beta", "gamma alpha", "delta"});
  view.set_query("alpha");
  EXPECT_EQ(view.status(), SearchStatus::kMatches);
  // Prime the virtual list's box (first frame uses a stale 1-row viewport).
  test_support::render_to_screen(view.component()->Render(), 30, 5);
  const std::string first = RenderText(view, 30, 5);
  const std::string second = RenderText(view, 30, 5);
  EXPECT_EQ(first, second);
  EXPECT_EQ(view.match_count(), 2u);  // unchanged by repeated renders
}

TEST(SearchableTextView, LongLineWrapsAtNarrowWidthWithoutClipping) {
  SearchableTextViewOptions opts;
  opts.show_line_numbers = false;
  SearchableTextView view(std::move(opts));
  // 70 ASCII characters -> wraps across multiple rows at width 20.
  const std::string long_line(70, 'x');
  view.set_lines({long_line});
  // Establish the narrow width (one frame sets the box, the next re-wraps).
  test_support::render_to_screen(view.component()->Render(), 20, 10);
  std::string narrow = RenderText(view, 20, 10);
  // Every character is reachable across wrapped rows.
  EXPECT_NE(narrow.find("xxxxx"), std::string::npos);
  // Resize to a wide terminal; the whole line now fits without clipping.
  test_support::render_to_screen(view.component()->Render(), 70, 2);
  std::string wide = RenderText(view, 70, 2);
  EXPECT_NE(wide.find(std::string(70, 'x')), std::string::npos);
}

TEST(SearchableTextView, MatchOnWrappedLineStillCountedAndFindable) {
  SearchableTextViewOptions opts;
  opts.show_line_numbers = false;
  SearchableTextView view(std::move(opts));
  std::vector<std::string> lines;
  lines.push_back(std::string(20, 'a') + " needle " + std::string(20, 'b'));
  view.set_lines(lines);
  view.set_query("needle");
  EXPECT_EQ(view.status(), SearchStatus::kMatches);
  EXPECT_EQ(view.match_count(), 1u);
  test_support::render_to_screen(view.component()->Render(), 20, 10);
  std::string text = RenderText(view, 20, 10);
  EXPECT_NE(text.find("needle"), std::string::npos);
}

TEST(SearchableTextView, RenderDoesNotRebuildSearchIndex) {
  // The match set is computed when the query is applied. Plain re-renders must
  // not re-run the search (which would re-fire the status callback). Count how
  // many times the status callback fires: once for the empty-query application
  // in set_lines, once more for set_query, then never again on re-render.
  int status_calls = 0;
  SearchableTextViewOptions opts;
  opts.on_status_change = [&status_calls](SearchStatus) { ++status_calls; };
  SearchableTextView view(std::move(opts));
  view.set_lines({"alpha beta", "alpha gamma"});
  const int after_set_lines = status_calls;  // empty query applied once

  view.set_query("alpha");
  const int after_query = status_calls;

  for (int i = 0; i < 5; ++i) {
    RenderText(view, 30, 5);
  }
  // Re-renders did not rebuild the search index.
  EXPECT_GT(after_query, after_set_lines);
  EXPECT_EQ(status_calls, after_query);
  EXPECT_EQ(view.match_count(), 2u);
}

TEST(SearchableTextView, KeyFlowSearchesAndNavigates) {
  SearchableTextView view;
  view.set_lines({"apple", "banana apple"});

  // "/" opens the search prompt.
  view.component()->OnEvent(ftxui::Event::Character('/'));
  EXPECT_TRUE(view.search_open());

  // Typing updates the query incrementally.
  view.component()->OnEvent(ftxui::Event::Character('a'));
  view.component()->OnEvent(ftxui::Event::Character('p'));
  view.component()->OnEvent(ftxui::Event::Character('p'));
  view.component()->OnEvent(ftxui::Event::Character('l'));
  EXPECT_EQ(view.query(), "appl");
  EXPECT_EQ(view.match_count(), 2u);  // live incremental results

  // Enter applies the query and closes the prompt.
  view.component()->OnEvent(ftxui::Event::Return);
  EXPECT_FALSE(view.search_open());
  EXPECT_EQ(view.query(), "appl");
  EXPECT_EQ(view.match_count(), 2u);

  // "n" advances to the next match.
  view.component()->OnEvent(ftxui::Event::Character('n'));
  ASSERT_TRUE(view.current_match_index().has_value());
  EXPECT_EQ(*view.current_match_index(), 1u);

  // "/" then Escape cancels and clears the query.
  view.component()->OnEvent(ftxui::Event::Character('/'));
  EXPECT_TRUE(view.search_open());
  view.component()->OnEvent(ftxui::Event::Escape);
  EXPECT_FALSE(view.search_open());
  EXPECT_EQ(view.query(), "");
  EXPECT_EQ(view.status(), SearchStatus::kEmptyQuery);
}

TEST(SearchableTextView, EscapeClearsQueryAndCloses) {
  SearchableTextView view;
  view.set_lines({"foo", "foobar", "bar"});
  view.open_search();
  EXPECT_TRUE(view.search_open());
  // Emulate typing "f" then "o" via set_query; then Escape cancels.
  view.set_query("foo");
  EXPECT_EQ(view.match_count(), 2u);
  view.component()->OnEvent(ftxui::Event::Escape);
  EXPECT_EQ(view.query(), "");
  EXPECT_EQ(view.status(), SearchStatus::kEmptyQuery);
  EXPECT_FALSE(view.search_open());
}

}  // namespace
}  // namespace terminal_ui_kit
