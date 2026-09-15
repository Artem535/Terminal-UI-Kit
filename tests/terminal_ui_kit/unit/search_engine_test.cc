#include "terminal_ui_kit/search/search_engine.h"

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

std::span<const std::string_view> ToSpan(std::vector<std::string_view>& lines) {
  return std::span<const std::string_view>(lines);
}

SearchStatus DoSearch(std::vector<std::string_view> lines, std::string_view query,
                      const SearchOptions& options, std::vector<TextMatch>& matches) {
  return SearchEngine::search(ToSpan(lines), query, options, matches);
}

TEST(SearchEngine, EmptyQueryYieldsEmptyQueryStatus) {
  std::vector<std::string_view> lines{"hello world"};
  std::vector<TextMatch> matches;
  SearchOptions options;
  EXPECT_EQ(DoSearch(lines, "", options, matches), SearchStatus::kEmptyQuery);
  EXPECT_TRUE(matches.empty());
}

TEST(SearchEngine, QueryLongerThanLineNoOutOfBounds) {
  // The case-insensitive path must not underflow when the query is longer
  // than a line (an out-of-bounds read would follow).
  std::vector<std::string_view> lines{"abc"};
  std::vector<TextMatch> matches;
  SearchOptions options;  // case-insensitive by default
  EXPECT_EQ(DoSearch(lines, "abcdef", options, matches), SearchStatus::kNoResults);
  EXPECT_TRUE(matches.empty());
}

TEST(SearchEngine, EmptyDocumentYieldsNoResults) {
  std::vector<std::string_view> lines;
  std::vector<TextMatch> matches;
  SearchOptions options;
  EXPECT_EQ(DoSearch(lines, "x", options, matches), SearchStatus::kNoResults);
  EXPECT_TRUE(matches.empty());
}

TEST(SearchEngine, OneMatchReportsByteOffsets) {
  std::vector<std::string_view> lines{"foo bar", "hello"};
  std::vector<TextMatch> matches;
  SearchOptions options;
  ASSERT_EQ(DoSearch(lines, "bar", options, matches), SearchStatus::kMatches);
  ASSERT_EQ(matches.size(), 1u);
  EXPECT_EQ(matches[0].line, 0u);
  EXPECT_EQ(matches[0].start_byte, 4u);
  EXPECT_EQ(matches[0].end_byte, 7u);
}

TEST(SearchEngine, MultipleMatchesOnOneLine) {
  std::vector<std::string_view> lines{"foo bar foo baz foo"};
  std::vector<TextMatch> matches;
  SearchOptions options;
  ASSERT_EQ(DoSearch(lines, "foo", options, matches), SearchStatus::kMatches);
  ASSERT_EQ(matches.size(), 3u);
  EXPECT_EQ(matches[0].start_byte, 0u);
  EXPECT_EQ(matches[1].start_byte, 8u);
  EXPECT_EQ(matches[2].start_byte, 16u);
}

TEST(SearchEngine, NonOverlappingMatches) {
  std::vector<std::string_view> lines{"aaaa"};
  std::vector<TextMatch> matches;
  SearchOptions options;
  ASSERT_EQ(DoSearch(lines, "aa", options, matches), SearchStatus::kMatches);
  // Matches at 0 and 2 only -- never overlapping.
  ASSERT_EQ(matches.size(), 2u);
  EXPECT_EQ(matches[0].start_byte, 0u);
  EXPECT_EQ(matches[1].start_byte, 2u);
}

TEST(SearchEngine, CaseInsensitiveByDefault) {
  std::vector<std::string_view> lines{"Foo BAR foo"};
  std::vector<TextMatch> matches;
  SearchOptions options;
  ASSERT_EQ(DoSearch(lines, "foo", options, matches), SearchStatus::kMatches);
  EXPECT_EQ(matches.size(), 2u);  // "Foo" and "foo"
}

TEST(SearchEngine, CaseSensitiveRequiresExactCase) {
  std::vector<std::string_view> lines{"Foo foo FOO"};
  std::vector<TextMatch> matches;
  SearchOptions options;
  options.case_sensitive = true;
  ASSERT_EQ(DoSearch(lines, "foo", options, matches), SearchStatus::kMatches);
  ASSERT_EQ(matches.size(), 1u);
  EXPECT_EQ(matches[0].start_byte, 4u);
}

TEST(SearchEngine, CaseSensitiveNoResult) {
  std::vector<std::string_view> lines{"FOO"};
  std::vector<TextMatch> matches;
  SearchOptions options;
  options.case_sensitive = true;
  EXPECT_EQ(DoSearch(lines, "foo", options, matches), SearchStatus::kNoResults);
}

TEST(SearchEngine, RegexMode) {
  std::vector<std::string_view> lines{"foo boo", "bar"};
  std::vector<TextMatch> matches;
  SearchOptions options;
  options.use_regex = true;
  ASSERT_EQ(DoSearch(lines, "[fb]oo", options, matches), SearchStatus::kMatches);
  ASSERT_EQ(matches.size(), 2u);
  EXPECT_EQ(matches[0].start_byte, 0u);
  EXPECT_EQ(matches[1].start_byte, 4u);
}

TEST(SearchEngine, RegexCaseInsensitive) {
  std::vector<std::string_view> lines{"HELLO world"};
  std::vector<TextMatch> matches;
  SearchOptions options;
  options.use_regex = true;  // icase by default
  ASSERT_EQ(DoSearch(lines, "hello", options, matches), SearchStatus::kMatches);
  ASSERT_EQ(matches.size(), 1u);
  EXPECT_EQ(matches[0].start_byte, 0u);
}

TEST(SearchEngine, InvalidRegexYieldsInvalidRegex) {
  std::vector<std::string_view> lines{"anything"};
  std::vector<TextMatch> matches;
  SearchOptions options;
  options.use_regex = true;
  EXPECT_EQ(DoSearch(lines, "(", options, matches), SearchStatus::kInvalidRegex);
  EXPECT_TRUE(matches.empty());
}

TEST(SearchEngine, ZeroLengthRegexMatchesSkipped) {
  // "a*" matches empty everywhere, but only non-empty matches are reported.
  std::vector<std::string_view> lines{"bbb"};
  std::vector<TextMatch> matches;
  SearchOptions options;
  options.use_regex = true;
  EXPECT_EQ(DoSearch(lines, "a*", options, matches), SearchStatus::kNoResults);
}

TEST(SearchEngine, Utf8MatchOffsetsAlignToCodePoints) {
  // "привет мир" -- UTF-8. Query "мир" is a valid UTF-8 substring; byte
  // offsets must land exactly on the code points, never inside a sequence.
  std::vector<std::string_view> lines{
      "\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82 \xD0\xBC\xD0\xB8\xD1\x80"};
  std::vector<TextMatch> matches;
  SearchOptions options;
  ASSERT_EQ(DoSearch(lines, "\xD0\xBC\xD0\xB8\xD1\x80", options, matches), SearchStatus::kMatches);
  ASSERT_EQ(matches.size(), 1u);
  // "привет " is 12 UTF-8 bytes, so "мир" starts at byte 13 and ends at 19.
  EXPECT_EQ(matches[0].start_byte, 13u);
  EXPECT_EQ(matches[0].end_byte, 19u);
}

TEST(SearchEngine, Utf8InlineAsciiCaseInsensitive) {
  // ASCII letters inside a UTF-8 line are folded; the Cyrillic text is
  // matched in exact case only.
  std::vector<std::string_view> lines{"HELLO \xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82"};
  std::vector<TextMatch> matches;
  SearchOptions options;
  ASSERT_EQ(DoSearch(lines, "hello", options, matches), SearchStatus::kMatches);
  EXPECT_EQ(matches[0].start_byte, 0u);
  EXPECT_EQ(matches[0].end_byte, 5u);
}

TEST(SearchEngine, LongLineOffsets) {
  std::string line;
  line.reserve(1200);
  for (int i = 0; i < 100; ++i) {
    line += "word ";
  }
  line += "needle";
  std::vector<std::string_view> lines{line};
  std::vector<TextMatch> matches;
  SearchOptions options;
  ASSERT_EQ(DoSearch(lines, "needle", options, matches), SearchStatus::kMatches);
  ASSERT_EQ(matches.size(), 1u);
  EXPECT_EQ(matches[0].end_byte, line.size());
  EXPECT_EQ(matches[0].start_byte, line.size() - 6u);
}

TEST(SearchEngine, StableSourceOffsets) {
  std::vector<std::string_view> lines{"foo foo foo"};
  std::vector<TextMatch> first;
  std::vector<TextMatch> second;
  SearchOptions options;
  ASSERT_EQ(DoSearch(lines, "foo", options, first), SearchStatus::kMatches);
  ASSERT_EQ(DoSearch(lines, "foo", options, second), SearchStatus::kMatches);
  ASSERT_EQ(first.size(), second.size());
  for (std::size_t i = 0; i < first.size(); ++i) {
    EXPECT_EQ(first[i], second[i]);
  }
}

TEST(SearchEngine, LargeDocumentManyMatches) {
  constexpr std::size_t kLines = 2000;
  std::vector<std::string> storage;
  storage.reserve(kLines);
  for (std::size_t i = 0; i < kLines; ++i) {
    storage.push_back("line " + std::to_string(i) + " target text");
  }
  std::vector<std::string_view> views;
  views.reserve(kLines);
  for (const std::string& s : storage) {
    views.push_back(s);
  }
  std::vector<TextMatch> matches;
  SearchOptions options;
  ASSERT_EQ(DoSearch(views, "target", options, matches), SearchStatus::kMatches);
  EXPECT_EQ(matches.size(), kLines);
  EXPECT_EQ(matches[0].line, 0u);
  EXPECT_EQ(matches.back().line, kLines - 1u);
}

// ---- MatchNavigator ----

TEST(MatchNavigator, EmptyInitially) {
  MatchNavigator nav;
  EXPECT_FALSE(nav.has_matches());
  EXPECT_EQ(nav.count(), 0u);
  EXPECT_FALSE(nav.current().has_value());
}

TEST(MatchNavigator, SetMatchesSelectsFirstWhenNoPriorCursor) {
  MatchNavigator nav;
  nav.set_matches({TextMatch{0, 1, 2}, TextMatch{1, 3, 4}});
  ASSERT_TRUE(nav.current().has_value());
  EXPECT_EQ(nav.current_index(), 0u);
  EXPECT_EQ(*nav.current(), (TextMatch{0, 1, 2}));
}

TEST(MatchNavigator, NextWrapsToFirst) {
  MatchNavigator nav;
  nav.set_matches({TextMatch{0, 0, 1}, TextMatch{1, 0, 1}, TextMatch{2, 0, 1}});
  nav.next();
  EXPECT_EQ(nav.current_index(), 1u);
  nav.next();
  EXPECT_EQ(nav.current_index(), 2u);
  nav.next();  // wraps
  EXPECT_EQ(nav.current_index(), 0u);
  EXPECT_EQ(*nav.current(), (TextMatch{0, 0, 1}));
}

TEST(MatchNavigator, PreviousWrapsToLast) {
  MatchNavigator nav;
  nav.set_matches({TextMatch{0, 0, 1}, TextMatch{1, 0, 1}, TextMatch{2, 0, 1}});
  nav.previous();  // wraps to last
  EXPECT_EQ(nav.current_index(), 2u);
  nav.previous();
  EXPECT_EQ(nav.current_index(), 1u);
  nav.previous();
  EXPECT_EQ(nav.current_index(), 0u);
}

TEST(MatchNavigator, ClearResets) {
  MatchNavigator nav;
  nav.set_matches({TextMatch{0, 0, 1}});
  nav.clear();
  EXPECT_FALSE(nav.has_matches());
  EXPECT_FALSE(nav.current().has_value());
}

TEST(MatchNavigator, SetCurrentClamps) {
  MatchNavigator nav;
  nav.set_matches({TextMatch{0, 0, 1}, TextMatch{1, 0, 1}});
  nav.set_current(10);
  EXPECT_EQ(nav.current_index(), 1u);
}

TEST(MatchNavigator, QueryChangeKeepsIdenticalMatch) {
  MatchNavigator nav;
  nav.set_matches({TextMatch{0, 0, 3}, TextMatch{1, 5, 8}, TextMatch{2, 0, 3}});
  nav.set_current(1u);  // active is {1,5,8}
  // Simulate a query change that removes {0,0,3} but keeps {1,5,8}.
  nav.set_matches({TextMatch{1, 5, 8}, TextMatch{2, 0, 3}});
  EXPECT_EQ(nav.current_index(), 0u);
  EXPECT_EQ(*nav.current(), (TextMatch{1, 5, 8}));
}

TEST(MatchNavigator, ResultCountShrinkClampsCursor) {
  MatchNavigator nav;
  nav.set_matches({TextMatch{0, 0, 1}, TextMatch{1, 0, 1}, TextMatch{2, 0, 1}});
  nav.set_current(2u);
  // New result set has only two matches; the previous active location is gone.
  nav.set_matches({TextMatch{5, 0, 1}, TextMatch{6, 0, 1}});
  ASSERT_TRUE(nav.current_index().has_value());
  EXPECT_EQ(*nav.current_index(), 1u);  // clamped into [0, count)
}

TEST(MatchNavigator, ShrinkToEmptyClearsCursor) {
  MatchNavigator nav;
  nav.set_matches({TextMatch{0, 0, 1}});
  nav.set_matches({});
  EXPECT_FALSE(nav.has_matches());
  EXPECT_FALSE(nav.current().has_value());
}

}  // namespace
}  // namespace terminal_ui_kit
