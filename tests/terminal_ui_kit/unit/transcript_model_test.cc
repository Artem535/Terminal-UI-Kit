#include "terminal_ui_kit/components/transcript_model.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "terminal_ui_kit/components/status.h"
#include "terminal_ui_kit/document/log_model.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TranscriptItem MakeText(std::string text) { return TextBlock{std::move(text)}; }

TEST(TranscriptModel, EmptyTranscriptHasNoBlocksAndNoMatches) {
  TranscriptModel model;
  EXPECT_EQ(model.block_count(), 0u);
  EXPECT_FALSE(model.has_tail());
  EXPECT_FALSE(model.tail_index().has_value());
  EXPECT_EQ(model.match_count(), 0u);
  EXPECT_FALSE(model.find("anything").has_value());
}

TEST(TranscriptModel, OneBlockAppendAndReadBack) {
  TranscriptModel model;
  const std::size_t index = model.append(MakeText("hello world"));
  EXPECT_EQ(index, 0u);
  EXPECT_EQ(model.block_count(), 1u);
  EXPECT_EQ(model.block_plain_text(0), "hello world");
  EXPECT_EQ(model.block_lines(0), (std::vector<std::string>{"hello world"}));
}

TEST(TranscriptModel, MixedBlockTypesAreStoredAndReported) {
  TranscriptModel model;
  model.append(MakeText("text"));
  model.append(MarkdownBlock{"# heading"});
  model.append(CodeBlock{"int x;\nreturn x;", "cpp"});
  model.append(LogBlock{LogSeverity::kError, "boom"});
  model.append(DiffBlock{"+added\n-removed"});
  model.append(StatusBlock{Status::kRunning, "working"});
  model.append(CustomBlock{"tool", "content lines\nsecond"});

  ASSERT_EQ(model.block_count(), 7u);
  EXPECT_EQ(model.block_plain_text(0), "text");
  EXPECT_EQ(model.block_plain_text(1), "# heading");
  EXPECT_EQ(model.block_plain_text(2), "int x;\nreturn x;");
  EXPECT_EQ(model.block_plain_text(3), "boom");
  EXPECT_EQ(model.block_plain_text(4), "+added\n-removed");
  EXPECT_EQ(model.block_plain_text(5), "working");
  EXPECT_EQ(model.block_plain_text(6), "content lines\nsecond");

  EXPECT_EQ(model.block_lines(2), (std::vector<std::string>{"int x;", "return x;"}));
}

TEST(TranscriptModel, BeginTailCreatesSingleStreamingBlock) {
  TranscriptModel model;
  model.append(MakeText("done"));
  const std::size_t tail = model.begin_tail(MakeText(""));
  EXPECT_TRUE(model.has_tail());
  EXPECT_EQ(model.tail_index(), std::optional<std::size_t>{tail});
  EXPECT_EQ(model.block_count(), 2u);
}

TEST(TranscriptModel, AppendTailDoesNotCreateNewEntries) {
  TranscriptModel model;
  model.append(MakeText("done"));
  model.begin_tail(MakeText(""));
  const std::size_t count_before = model.block_count();

  for (int i = 0; i < 100; ++i) {
    model.append_tail("chunk ");
  }
  EXPECT_EQ(model.block_count(), count_before);
  EXPECT_TRUE(model.has_tail());
  // All chunks accumulate into the single tail block.
  std::string expected;
  for (int i = 0; i < 100; ++i) {
    expected += "chunk ";
  }
  EXPECT_EQ(model.block_plain_text(1), expected);
}

TEST(TranscriptModel, ReplaceTailRewritesContentInPlace) {
  TranscriptModel model;
  model.begin_tail(MakeText("stale"));
  model.replace_tail("fresh");
  EXPECT_EQ(model.block_plain_text(model.block_count() - 1), "fresh");
  EXPECT_TRUE(model.has_tail());
  EXPECT_EQ(model.block_count(), 1u);
}

TEST(TranscriptModel, FinalizeTailPromotesToCompletedBlock) {
  TranscriptModel model;
  model.append(MakeText("a"));
  const std::size_t tail = model.begin_tail(MakeText("streaming"));
  model.append_tail(" extra");
  model.finalize_tail();

  EXPECT_FALSE(model.has_tail());
  EXPECT_FALSE(model.tail_index().has_value());
  EXPECT_EQ(model.block_count(), 2u);
  EXPECT_EQ(model.block_plain_text(tail), "streaming extra");
}

TEST(TranscriptModel, BeginTailFinalizesPreviousTail) {
  TranscriptModel model;
  model.begin_tail(MakeText("first"));
  model.append_tail(" tail");
  const std::size_t second = model.begin_tail(MakeText("second"));
  EXPECT_EQ(model.block_count(), 2u);
  EXPECT_EQ(model.tail_index(), std::optional<std::size_t>{second});
  // First tail was finalized and keeps its content.
  EXPECT_EQ(model.block_plain_text(0), "first tail");
  EXPECT_EQ(model.block_plain_text(1), "second");
}

TEST(TranscriptModel, SearchFindsAndNavigatesMatchesCaseInsensitively) {
  TranscriptModel model;
  model.append(MakeText("alpha beta"));
  model.append(MakeText("gamma"));
  model.append(MakeText("ALPHA omega"));
  model.append(MakeText("delta"));

  const std::optional<std::size_t> first = model.find("alpha");
  ASSERT_TRUE(first.has_value());
  EXPECT_EQ(*first, 0u);
  EXPECT_EQ(model.match_count(), 2u);

  EXPECT_EQ(model.next_match(0), std::optional<std::size_t>{2});
  EXPECT_EQ(model.next_match(2), std::nullopt);
  EXPECT_EQ(model.previous_match(2), std::optional<std::size_t>{0});
  EXPECT_EQ(model.previous_match(0), std::nullopt);
}

TEST(TranscriptModel, SearchWithEmptyOrAbsentQueryHasNoMatches) {
  TranscriptModel model;
  model.append(MakeText("nothing here"));
  EXPECT_FALSE(model.find("").has_value());
  EXPECT_EQ(model.match_count(), 0u);
  EXPECT_FALSE(model.find("zzz").has_value());
}

TEST(TranscriptModel, SearchIndexIsCachedAcrossQueriesUntilRevisionChanges) {
  TranscriptModel model;
  model.append(MakeText("apple"));
  model.append(MakeText("banana"));

  // Same query twice returns immediately without a rebuild; the cached
  // result is stable and correct.
  ASSERT_TRUE(model.find("banana").has_value());  // warms the cache
  EXPECT_EQ(model.match_count(), 1u);
  EXPECT_EQ(model.find("banana"), std::optional<std::size_t>{1});

  // A data revision change invalidates the cache.
  model.append(MakeText("pineapple"));
  EXPECT_EQ(model.find("apple"), std::optional<std::size_t>{0});
  EXPECT_EQ(model.match_count(), 2u);
}

TEST(TranscriptModel, BookmarkToggleAddsAndRemoves) {
  TranscriptModel model;
  model.append(MakeText("a"));
  model.append(MakeText("b"));
  model.append(MakeText("c"));

  model.toggle_bookmark(0);
  model.toggle_bookmark(2);
  EXPECT_TRUE(model.is_bookmarked(0));
  EXPECT_FALSE(model.is_bookmarked(1));
  EXPECT_TRUE(model.is_bookmarked(2));
  EXPECT_EQ(model.bookmarks(), (std::vector<std::size_t>{0, 2}));

  model.toggle_bookmark(0);
  EXPECT_FALSE(model.is_bookmarked(0));
  EXPECT_EQ(model.bookmarks(), (std::vector<std::size_t>{2}));
}

TEST(TranscriptModel, ClearResetsBlocksTailSearchAndBookmarks) {
  TranscriptModel model;
  model.append(MakeText("one"));
  model.append(MakeText("searchable"));
  model.begin_tail(MakeText("tail"));
  model.toggle_bookmark(0);
  model.find("search");

  model.clear();

  EXPECT_EQ(model.block_count(), 0u);
  EXPECT_FALSE(model.has_tail());
  EXPECT_FALSE(model.is_bookmarked(0));
  EXPECT_EQ(model.match_count(), 0u);
}

TEST(TranscriptModel, SupportsOneHundredThousandBlocks) {
  TranscriptModel model;
  for (std::size_t i = 0; i < 100000; ++i) {
    model.append(MakeText("message number " + std::to_string(i)));
  }
  EXPECT_EQ(model.block_count(), 100000u);
  EXPECT_EQ(model.block_plain_text(0), "message number 0");
  EXPECT_EQ(model.block_plain_text(99999), "message number 99999");

  // Search across the full set finds scattered matches.
  const std::optional<std::size_t> first = model.find("99999");
  ASSERT_TRUE(first.has_value());
  EXPECT_EQ(*first, 99999u);
}

TEST(TranscriptModel, OutOfRangeBlockAtThrows) {
  TranscriptModel model;
  model.append(MakeText("a"));
  EXPECT_THROW((void)model.block_at(5), std::out_of_range);
  EXPECT_THROW((void)model.block_plain_text(1), std::out_of_range);
  EXPECT_THROW((void)model.block_lines(7), std::out_of_range);
}

TEST(TranscriptModel, AppendWhileTailActiveFinalizesTailFirst) {
  TranscriptModel model;
  // begin_tail appends the tail, then append() finalizes it and appends a
  // completed block at the very end, so the tail never stays mid-list.
  model.begin_tail(MakeText("streaming"));
  const std::size_t completed_index = model.append(MakeText("completed"));
  EXPECT_FALSE(model.has_tail());
  EXPECT_EQ(model.block_count(), 2u);
  EXPECT_EQ(completed_index, 1u);
  EXPECT_EQ(model.block_plain_text(0), "streaming");
  EXPECT_EQ(model.block_plain_text(1), "completed");
}

TEST(TranscriptModel, TailVisibleLinesReturnsLastLinesWithoutSplittingWholeTail) {
  TranscriptModel model;
  model.begin_tail(MakeText(""));
  for (int i = 0; i < 100; ++i) {
    model.append_tail("line" + std::to_string(i) + "\n");
  }

  // The tail text is large; only the trailing 3 lines must be returned.
  const std::vector<std::string> visible = model.tail_visible_lines(3);
  ASSERT_EQ(visible.size(), 3u);
  EXPECT_EQ(visible[0], "line97");
  EXPECT_EQ(visible[1], "line98");
  EXPECT_EQ(visible[2], "line99");
}

TEST(TranscriptModel, TailVisibleLinesEdges) {
  TranscriptModel model;
  // No active tail → empty.
  EXPECT_TRUE(model.tail_visible_lines(4).empty());

  model.begin_tail(MakeText("one\ntwo\nthree"));
  // Fewer lines than the window → all lines, in order.
  EXPECT_EQ(model.tail_visible_lines(10), (std::vector<std::string>{"one", "two", "three"}));
  // A trailing newline does not add a phantom blank last line.
  model.replace_tail("a\nb\n");
  EXPECT_EQ(model.tail_visible_lines(10), (std::vector<std::string>{"a", "b"}));
  // Empty tail content → empty.
  model.replace_tail("");
  EXPECT_TRUE(model.tail_visible_lines(4).empty());
}

TEST(TranscriptModel, SplitLinesDoesNotEmitPhantomTrailingEmpty) {
  TranscriptModel model;
  model.append(MakeText("a\nb\n"));
  // One trailing newline must not create a phantom blank display line.
  EXPECT_EQ(model.block_lines(0), (std::vector<std::string>{"a", "b"}));
}

}  // namespace
}  // namespace terminal_ui_kit