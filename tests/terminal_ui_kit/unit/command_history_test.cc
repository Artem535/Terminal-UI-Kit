// Tests for terminal_ui_kit::command::CommandHistory — bounded, navigable
// command history, its persistence adapter contract, and sensitive-command
// policy.

#include "terminal_ui_kit/command/command_history.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace command {
namespace {

// In-memory persistence double. Records every saved command and can be told to
// fail the next N Save()/Clear() calls so failure paths are observable.
class FakePersistence final : public CommandHistoryPersistence {
 public:
  bool Save(const std::string& command) override {
    if (fail_saves_ > 0) {
      --fail_saves_;
      return false;
    }
    saved_.push_back(command);
    return true;
  }

  bool Clear() override {
    if (fail_clears_ > 0) {
      --fail_clears_;
      return false;
    }
    saved_.clear();
    return true;
  }

  void FailNextSaves(std::size_t count) { fail_saves_ = count; }
  void FailNextClear() { fail_clears_ = 1; }

  std::vector<std::string> saved_;
  std::size_t fail_saves_ = 0;
  std::size_t fail_clears_ = 0;
};

// Asserts that `actual` equals `expected` (chronological order preserved).
void ExpectCommands(const std::vector<std::string>& actual,
                    const std::vector<std::string>& expected) {
  EXPECT_EQ(actual, expected);
}

std::string AddAndReturn(std::string base) { return base + "-suffix"; }

// ---- Empty history -------------------------------------------------------

TEST(CommandHistoryTest, EmptyHistory) {
  CommandHistory history(10);
  EXPECT_EQ(history.Size(), 0u);
  EXPECT_EQ(history.Capacity(), 10u);
  EXPECT_FALSE(history.Previous().has_value());
  EXPECT_FALSE(history.Next().has_value());
  ExpectCommands(history.Search("x"), {});
  ExpectCommands(history.SearchPrefix("x"), {});
}

// ---- Capacity 0 ----------------------------------------------------------

TEST(CommandHistoryTest, CapacityZeroDisablesRetention) {
  CommandHistory history(0);
  EXPECT_EQ(history.Capacity(), 0u);

  history.Add("ls");
  history.Add("  ");
  EXPECT_EQ(history.Size(), 0u);
  EXPECT_FALSE(history.Previous().has_value());
  EXPECT_FALSE(history.Next().has_value());
}

TEST(CommandHistoryTest, CapacityZeroStillRejectsBlank) {
  CommandHistory history(0);
  history.Add(" \t ");
  EXPECT_EQ(history.Size(), 0u);
}

// ---- One item ------------------------------------------------------------

TEST(CommandHistoryTest, OneItem) {
  CommandHistory history(10);
  history.Add("hello");
  ASSERT_EQ(history.Size(), 1u);
  EXPECT_EQ(history.Previous(), std::optional<std::string>("hello"));
  // Boundary: cursor is at the oldest (and only) entry; Previous returns it.
  EXPECT_EQ(history.Previous(), std::optional<std::string>("hello"));
  // Next moves past the newest entry.
  EXPECT_FALSE(history.Next().has_value());
}

// ---- Previous / Next navigation ------------------------------------------

TEST(CommandHistoryTest, NavigationTraversesNewestToOldest) {
  CommandHistory history(10);
  history.Add("a");
  history.Add("b");
  history.Add("c");

  // Add() leaves the cursor at the end; Previous walks newest -> oldest.
  EXPECT_EQ(history.Previous(), std::optional<std::string>("c"));
  EXPECT_EQ(history.Previous(), std::optional<std::string>("b"));
  EXPECT_EQ(history.Previous(), std::optional<std::string>("a"));
  // Beginning boundary: remains at the oldest entry.
  EXPECT_EQ(history.Previous(), std::optional<std::string>("a"));
}

TEST(CommandHistoryTest, NextNavigatesBackTowardsNewest) {
  CommandHistory history(10);
  history.Add("a");
  history.Add("b");
  history.Add("c");

  EXPECT_EQ(history.Previous(), std::optional<std::string>("c"));
  EXPECT_FALSE(history.Next().has_value());  // already past newest
  EXPECT_EQ(history.Previous(), std::optional<std::string>("c"));

  EXPECT_EQ(history.Previous(), std::optional<std::string>("b"));
  EXPECT_EQ(history.Next(), std::optional<std::string>("c"));
  EXPECT_FALSE(history.Next().has_value());
}

// ---- Start / end boundaries ----------------------------------------------

TEST(CommandHistoryTest, NextAtNewestReturnsNulloptAndStays) {
  CommandHistory history(10);
  history.Add("only");
  EXPECT_EQ(history.Previous(), std::optional<std::string>("only"));
  EXPECT_FALSE(history.Next().has_value());
  // Cursor did not wrap: navigation still deterministic.
  EXPECT_EQ(history.Previous(), std::optional<std::string>("only"));
}

// ---- Adding after navigation ---------------------------------------------

TEST(CommandHistoryTest, AddingResetsNavigationCursor) {
  CommandHistory history(10);
  history.Add("a");
  history.Add("b");
  EXPECT_EQ(history.Previous(), std::optional<std::string>("b"));
  EXPECT_EQ(history.Previous(), std::optional<std::string>("a"));

  // Adding a new command must reset the cursor to the end.
  history.Add("c");
  EXPECT_EQ(history.Previous(), std::optional<std::string>("c"));
}

// ---- Capacity eviction ---------------------------------------------------

TEST(CommandHistoryTest, EvictsOldestWhenFull) {
  CommandHistory history(3);
  history.Add("1");
  history.Add("2");
  history.Add("3");
  history.Add("4");  // evicts "1"

  ASSERT_EQ(history.Size(), 3u);
  ExpectCommands(history.Search(""), {"2", "3", "4"});
  EXPECT_EQ(history.Previous(), std::optional<std::string>("4"));
  EXPECT_EQ(history.Previous(), std::optional<std::string>("3"));
  EXPECT_EQ(history.Previous(), std::optional<std::string>("2"));
}

// ---- Consecutive duplicates ----------------------------------------------

TEST(CommandHistoryTest, ConsecutiveDuplicatesDropped) {
  CommandHistory history(10);
  history.Add("same");
  history.Add("same");
  history.Add("different");
  history.Add("same");  // not consecutive now -> allowed

  ASSERT_EQ(history.Size(), 3u);
  ExpectCommands(history.Search(""), {"same", "different", "same"});
}

// ---- Non-consecutive duplicates ------------------------------------------

TEST(CommandHistoryTest, NonConsecutiveDuplicatesAllowed) {
  CommandHistory history(10);
  history.Add("x");
  history.Add("y");
  history.Add("x");
  history.Add("y");

  ASSERT_EQ(history.Size(), 4u);
  ExpectCommands(history.Search(""), {"x", "y", "x", "y"});
}

// ---- Empty and whitespace-only input -------------------------------------

TEST(CommandHistoryTest, EmptyAndWhitespaceOnlyInputIgnored) {
  CommandHistory history(10);
  history.Add("");
  history.Add("   ");
  history.Add("\t");
  history.Add("\n");
  history.Add("real");
  EXPECT_EQ(history.Size(), 1u);
  EXPECT_EQ(history.Previous(), std::optional<std::string>("real"));
}

// ---- Substring search ----------------------------------------------------

TEST(CommandHistoryTest, SubstringSearch) {
  CommandHistory history(10);
  history.Add("user info");
  history.Add("list users");
  history.Add("get_user");
  history.Add("other");

  ExpectCommands(history.Search("user"), {"user info", "list users", "get_user"});
}

// ---- Prefix search -------------------------------------------------------

TEST(CommandHistoryTest, PrefixSearch) {
  CommandHistory history(10);
  history.Add("git status");
  history.Add("git push");
  history.Add("sudo npm i");
  history.Add("git");
  history.Add("gotcha");

  ExpectCommands(history.SearchPrefix("git"), {"git status", "git push", "git"});
}

// ---- Result ordering -----------------------------------------------------

TEST(CommandHistoryTest, SearchResultsChronologicalOldestFirst) {
  CommandHistory history(10);
  history.Add("apple");
  history.Add("banana");
  history.Add("pear");
  history.Add("anaconda");
  history.Add("grape");

  ExpectCommands(history.Search("an"), {"banana", "anaconda"});
  ExpectCommands(history.SearchPrefix("an"), {"anaconda"});
}

// ---- Clear ---------------------------------------------------------------

TEST(CommandHistoryTest, ClearResetsStorageAndNavigation) {
  CommandHistory history(10);
  history.Add("a");
  history.Add("b");
  history.Previous();
  history.Clear();

  EXPECT_EQ(history.Size(), 0u);
  EXPECT_FALSE(history.Previous().has_value());
  EXPECT_FALSE(history.Next().has_value());

  history.Add("c");
  EXPECT_EQ(history.Previous(), std::optional<std::string>("c"));
}

// ---- Persistence mock ----------------------------------------------------

TEST(CommandHistoryTest, PersistenceReceivesAddedCommands) {
  auto persistence = std::make_shared<FakePersistence>();
  CommandHistory history(10, persistence);
  history.Add("ls");
  history.Add("pwd");

  ExpectCommands(persistence->saved_, {"ls", "pwd"});
}

TEST(CommandHistoryTest, ClearForwardsToPersistence) {
  auto persistence = std::make_shared<FakePersistence>();
  CommandHistory history(10, persistence);
  history.Add("ls");
  ASSERT_EQ(persistence->saved_.size(), 1u);

  history.Clear();
  ExpectCommands(persistence->saved_, {});
}

// ---- Persistence failure -------------------------------------------------

TEST(CommandHistoryTest, PersistenceFailureLeavesHistoryUntouched) {
  auto persistence = std::make_shared<FakePersistence>();
  CommandHistory history(10, persistence);

  persistence->FailNextSaves(2);
  history.Add("first");   // Save fails
  history.Add("second");  // Save fails
  history.Add("third");   // Save succeeds

  // In-memory history is intact regardless of persistence failures.
  ASSERT_EQ(history.Size(), 3u);
  ExpectCommands(history.Search(""), {"first", "second", "third"});
  ExpectCommands(persistence->saved_, {"third"});
}

TEST(CommandHistoryTest, PersistenceClearFailureLeavesHistoryEmpty) {
  auto persistence = std::make_shared<FakePersistence>();
  CommandHistory history(10, persistence);
  history.Add("x");

  persistence->FailNextClear();
  history.Clear();

  // In-memory history is empty regardless of the adapter's clear result.
  EXPECT_EQ(history.Size(), 0u);
  ExpectCommands(history.Search(""), {});
}

// ---- Sensitive commands --------------------------------------------------

TEST(CommandHistoryTest, SensitiveCommandsNotPersistedButKeptInMemory) {
  auto persistence = std::make_shared<FakePersistence>();
  auto policy = std::make_shared<PrefixSensitivePolicy>("secret:");
  CommandHistory history(10, persistence, policy);

  history.Add("secret:hunter2");
  history.Add("ls");

  ExpectCommands(persistence->saved_, {"ls"});
  ASSERT_EQ(history.Size(), 2u);
  ExpectCommands(history.Search(""), {"secret:hunter2", "ls"});
}

TEST(CommandHistoryTest, SensitivePolicyCanBeSwapped) {
  auto persistence = std::make_shared<FakePersistence>();
  CommandHistory history(10, persistence, std::make_shared<NeverSensitivePolicy>());

  history.SetSensitivePolicy(std::make_shared<PrefixSensitivePolicy>("secret:"));
  history.Add("secret:hunter2");  // now sensitive -> not persisted
  history.SetSensitivePolicy(std::make_shared<NeverSensitivePolicy>());
  history.Add("secret:hunter3");  // not sensitive -> persisted

  ExpectCommands(persistence->saved_, {"secret:hunter3"});
  EXPECT_EQ(history.Size(), 2u);
}

TEST(CommandHistoryTest, NeverSensitivePolicyPersistsEverything) {
  auto persistence = std::make_shared<FakePersistence>();
  CommandHistory history(10, persistence, std::make_shared<NeverSensitivePolicy>());
  history.Add("anything");
  ExpectCommands(persistence->saved_, {"anything"});
}

// ---- Ownership / lifetime ------------------------------------------------

TEST(CommandHistoryTest, RetainsOwnedCopyAfterSourceDies) {
  CommandHistory history(10);
  {
    std::string ephemeral = AddAndReturn("doomed");
    history.Add(ephemeral);
  }  // `ephemeral` is destroyed here.
  EXPECT_EQ(history.Previous(), std::optional<std::string>("doomed-suffix"));
}

TEST(CommandHistoryTest, ReplacingPersistenceIsSafe) {
  CommandHistory history(10);
  auto first = std::make_shared<FakePersistence>();
  history.SetPersistence(first);
  history.Add("a");
  ASSERT_EQ(first->saved_.size(), 1u);

  auto second = std::make_shared<FakePersistence>();
  history.SetPersistence(second);
  history.Add("b");
  ExpectCommands(first->saved_, {"a"});   // untouched
  ExpectCommands(second->saved_, {"b"});  // new adapter used
  EXPECT_EQ(history.Size(), 2u);
}

}  // namespace
}  // namespace command
}  // namespace terminal_ui_kit