#include "terminal_ui_kit/editor/command_history.h"

#include <string>

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TEST(CommandHistory, EmptyByDefault) {
  CommandHistory history;
  EXPECT_TRUE(history.empty());
  EXPECT_EQ(history.size(), 0U);
}

TEST(CommandHistory, AddStoresOwnedEntriesNewestLast) {
  CommandHistory history;
  history.add("one");
  history.add("two");
  history.add("three");
  EXPECT_EQ(history.size(), 3U);
  EXPECT_EQ(history.at(0), "one");
  EXPECT_EQ(history.at(1), "two");
  EXPECT_EQ(history.at(2), "three");
}

TEST(CommandHistory, SkipsEmptyEntries) {
  CommandHistory history;
  history.add("");
  history.add("  ");
  history.add("real");
  EXPECT_EQ(history.size(), 1U);
  EXPECT_EQ(history.at(0), "real");
}

TEST(CommandHistory, DeduplicatesConsecutiveEntries) {
  CommandHistory history;
  history.add("same");
  history.add("same");
  history.add("other");
  history.add("same");
  EXPECT_EQ(history.size(), 3U);
  EXPECT_EQ(history.at(0), "same");
  EXPECT_EQ(history.at(1), "other");
  EXPECT_EQ(history.at(2), "same");
}

TEST(CommandHistory, RingBufferDropsOldestBeyondMax) {
  CommandHistory history(3);
  history.add("a");
  history.add("b");
  history.add("c");
  history.add("d");
  EXPECT_EQ(history.size(), 3U);
  EXPECT_EQ(history.at(0), "b");
  EXPECT_EQ(history.at(1), "c");
  EXPECT_EQ(history.at(2), "d");
}

TEST(CommandHistory, PreviousWalksBackThenNextWalksForward) {
  CommandHistory history;
  history.add("a");
  history.add("b");
  history.add("c");

  history.set_draft("draft");
  std::string out;
  EXPECT_TRUE(history.navigating() == false);
  EXPECT_TRUE(history.previous(out));
  EXPECT_EQ(out, "c");
  EXPECT_TRUE(history.previous(out));
  EXPECT_EQ(out, "b");
  EXPECT_TRUE(history.previous(out));
  EXPECT_EQ(out, "a");
  // Staying at the oldest entry.
  EXPECT_TRUE(history.previous(out));
  EXPECT_EQ(out, "a");

  EXPECT_TRUE(history.next(out));
  EXPECT_EQ(out, "b");
  EXPECT_TRUE(history.next(out));
  EXPECT_EQ(out, "c");
  // Newest restored the draft and exited navigation.
  EXPECT_TRUE(history.next(out));
  EXPECT_EQ(out, "draft");
  EXPECT_FALSE(history.navigating());
}

TEST(CommandHistory, PreviousEmptyReturnsFalse) {
  CommandHistory history;
  std::string out;
  EXPECT_FALSE(history.previous(out));
}

TEST(CommandHistory, NextWhenNotNavigatingReturnsFalse) {
  CommandHistory history;
  history.add("a");
  std::string out;
  EXPECT_FALSE(history.next(out));
}

TEST(CommandHistory, AddExitsNavigationAndClearsDraft) {
  CommandHistory history;
  history.add("a");
  history.add("b");
  history.set_draft("draft");
  std::string out;
  EXPECT_TRUE(history.previous(out));
  EXPECT_EQ(out, "b");
  history.add("c");  // a new submission resets navigation
  EXPECT_FALSE(history.navigating());
  EXPECT_FALSE(history.next(out));
}

TEST(CommandHistory, EndNavigationClearsDraft) {
  CommandHistory history;
  history.add("a");
  history.set_draft("draft");
  std::string out;
  history.previous(out);
  history.end_navigation();
  EXPECT_FALSE(history.navigating());
  // Starting a fresh recall should not restore an old draft at the newest.
  history.previous(out);
  EXPECT_EQ(out, "a");
  EXPECT_TRUE(history.next(out));
  EXPECT_EQ(out, "");  // draft was cleared
  EXPECT_FALSE(history.navigating());
}

TEST(CommandHistory, ClearEmptiesHistory) {
  CommandHistory history;
  history.add("a");
  history.add("b");
  history.clear();
  EXPECT_TRUE(history.empty());
  std::string out;
  EXPECT_FALSE(history.previous(out));
}

}  // namespace
}  // namespace terminal_ui_kit
