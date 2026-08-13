#include "terminal_ui_kit/editor/command_history.h"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace editor {
namespace {

// A store that records every command it is asked to persist. Used both to
// assert what reached the persistence layer and to drive failure cases.
class RecordingStore : public CommandHistoryStore {
 public:
  void Persist(const std::string& command) override { persisted_.push_back(command); }

  const std::vector<std::string>& persisted() const { return persisted_; }

 private:
  std::vector<std::string> persisted_;
};

// A store whose every Persist call throws, to prove a failing store never
// corrupts the in-memory history.
class ThrowingStore : public CommandHistoryStore {
 public:
  void Persist(const std::string&) override { throw std::runtime_error("store failure"); }
};

TEST(CommandHistory, EmptyHistoryHasZeroSizeAndNoNavigation) {
  CommandHistory history(10);
  EXPECT_EQ(history.Size(), 0u);
  EXPECT_EQ(history.Capacity(), 10u);
  EXPECT_EQ(history.Previous(), std::nullopt);
  EXPECT_EQ(history.Next(), std::nullopt);
  EXPECT_EQ(history.Current(), std::nullopt);
  EXPECT_TRUE(history.Search("").empty());
  EXPECT_TRUE(history.SearchPrefix("").empty());
}

TEST(CommandHistory, CapacityZeroStoresNothingSafely) {
  CommandHistory history(0);
  history.Add("ls");
  history.Add("");
  EXPECT_EQ(history.Size(), 0u);
  EXPECT_EQ(history.Capacity(), 0u);
  EXPECT_EQ(history.Previous(), std::nullopt);
  EXPECT_EQ(history.Next(), std::nullopt);
  EXPECT_TRUE(history.Search("ls").empty());
  EXPECT_TRUE(history.SearchPrefix("l").empty());
  history.Clear();  // Must be safe too.
  EXPECT_EQ(history.Size(), 0u);
}

TEST(CommandHistory, OneItem) {
  CommandHistory history(10);
  history.Add("alpha");
  ASSERT_EQ(history.Size(), 1u);
  EXPECT_EQ(history.Previous(), "alpha");
  // Previous at the only item stays clamped on it.
  EXPECT_EQ(history.Previous(), "alpha");
}

TEST(CommandHistory, PreviousWalksNewestToOldest) {
  CommandHistory history(10);
  history.Add("a");
  history.Add("b");
  history.Add("c");
  EXPECT_EQ(history.Previous(), "c");
  EXPECT_EQ(history.Previous(), "b");
  EXPECT_EQ(history.Previous(), "a");
}

TEST(CommandHistory, PreviousClampsAtOldestBoundary) {
  CommandHistory history(10);
  history.Add("a");
  history.Add("b");
  EXPECT_EQ(history.Previous(), "b");
  EXPECT_EQ(history.Previous(), "a");
  // Boundary: repeated Previous stays on the oldest command.
  EXPECT_EQ(history.Previous(), "a");
  EXPECT_EQ(history.Previous(), "a");
}

TEST(CommandHistory, NextWalksBackTowardNewest) {
  CommandHistory history(10);
  history.Add("a");
  history.Add("b");
  history.Add("c");
  EXPECT_EQ(history.Previous(), "c");
  EXPECT_EQ(history.Previous(), "b");
  EXPECT_EQ(history.Next(), "c");
}

TEST(CommandHistory, NextReturnsNulloptAtNewestAndResetsCursor) {
  CommandHistory history(10);
  history.Add("a");
  history.Add("b");
  EXPECT_EQ(history.Previous(), "b");
  // At the most recent command, Next returns to the draft position (nullopt).
  EXPECT_EQ(history.Next(), std::nullopt);
  EXPECT_EQ(history.Current(), std::nullopt);
  // The cursor is now reset, so a fresh Previous starts from the newest again.
  EXPECT_EQ(history.Previous(), "b");
}

TEST(CommandHistory, NextOnEmptyHistoryReturnsNullopt) {
  CommandHistory history(10);
  EXPECT_EQ(history.Next(), std::nullopt);
}

TEST(CommandHistory, AddingAfterNavigationResetsCursor) {
  CommandHistory history(10);
  history.Add("a");
  history.Add("b");
  history.Add("c");
  EXPECT_EQ(history.Previous(), "c");
  EXPECT_EQ(history.Previous(), "b");
  // Adding a new command resets the cursor to the top/draft.
  history.Add("d");
  EXPECT_EQ(history.Current(), std::nullopt);
  // Previous() now starts from the newest command again.
  EXPECT_EQ(history.Previous(), "d");
}

TEST(CommandHistory, CapacityEvictionDropsOldest) {
  CommandHistory history(2);
  history.Add("one");
  history.Add("two");
  EXPECT_EQ(history.Size(), 2u);
  history.Add("three");
  ASSERT_EQ(history.Size(), 2u);
  // Oldest ("one") was evicted; newest is "three".
  EXPECT_EQ(history.Previous(), "three");
  EXPECT_EQ(history.Previous(), "two");
  EXPECT_EQ(history.Previous(), "two");  // Clamped at the oldest remaining.
}

TEST(CommandHistory, ConsecutiveDuplicatesIgnored) {
  CommandHistory history(10);
  history.Add("ls");
  history.Add("ls");
  history.Add("ls");
  ASSERT_EQ(history.Size(), 1u);
  EXPECT_EQ(history.Previous(), "ls");
}

TEST(CommandHistory, NonConsecutiveDuplicatesKept) {
  CommandHistory history(10);
  history.Add("a");
  history.Add("b");
  history.Add("a");
  ASSERT_EQ(history.Size(), 3u);
  // Newest first: a, b, a.
  EXPECT_EQ(history.Previous(), "a");
  EXPECT_EQ(history.Previous(), "b");
  EXPECT_EQ(history.Previous(), "a");
}

TEST(CommandHistory, BlankInputIgnored) {
  CommandHistory history(10);
  history.Add("");
  history.Add("   ");
  history.Add("\t");
  history.Add(" \t ");
  ASSERT_EQ(history.Size(), 0u);

  // Real commands around blanks are kept; blanks never create duplicates.
  history.Add("ls");
  history.Add("  ");
  history.Add("grep");
  ASSERT_EQ(history.Size(), 2u);
}

TEST(CommandHistory, SubstringSearchFindsMatchesMostRecentFirst) {
  CommandHistory history(10);
  history.Add("git status");
  history.Add("git log");
  history.Add("grep error");
  history.Add("git commit");
  const std::vector<std::string> results = history.Search("git");
  const std::vector<std::string> expected = {"git commit", "git log", "git status"};
  EXPECT_EQ(results, expected);
}

TEST(CommandHistory, SubstringSearchEmptyQueryMatchesAll) {
  CommandHistory history(10);
  history.Add("a");
  history.Add("b");
  history.Add("c");
  const std::vector<std::string> results = history.Search("");
  const std::vector<std::string> expected = {"c", "b", "a"};
  EXPECT_EQ(results, expected);
}

TEST(CommandHistory, SubstringSearchNoMatchReturnsEmpty) {
  CommandHistory history(10);
  history.Add("git status");
  history.Add("git log");
  EXPECT_TRUE(history.Search("zzz").empty());
}

TEST(CommandHistory, PrefixSearchFindsMatchingPrefixesMostRecentFirst) {
  CommandHistory history(10);
  history.Add("git status");
  history.Add("git log");
  history.Add("grep error");
  history.Add("git commit");
  const std::vector<std::string> results = history.SearchPrefix("git");
  const std::vector<std::string> expected = {"git commit", "git log", "git status"};
  EXPECT_EQ(results, expected);
  // "grep error" does not start with "git", so it is excluded.
}

TEST(CommandHistory, PrefixSearchEmptyPrefixMatchesAll) {
  CommandHistory history(10);
  history.Add("a");
  history.Add("b");
  const std::vector<std::string> results = history.SearchPrefix("");
  const std::vector<std::string> expected = {"b", "a"};
  EXPECT_EQ(results, expected);
}

TEST(CommandHistory, SearchResultOrderingIsStableAndMostRecentFirst) {
  CommandHistory history(10);
  history.Add("run");
  history.Add("run test");
  history.Add("run");
  history.Add("run test again");
  const std::vector<std::string> substring = history.Search("test");
  const std::vector<std::string> expected_sub = {"run test again", "run test"};
  EXPECT_EQ(substring, expected_sub);

  const std::vector<std::string> prefix = history.SearchPrefix("run test");
  const std::vector<std::string> expected_pref = {"run test again", "run test"};
  EXPECT_EQ(prefix, expected_pref);
}

TEST(CommandHistory, ClearResetsStorageAndNavigation) {
  CommandHistory history(10);
  history.Add("a");
  history.Add("b");
  EXPECT_EQ(history.Previous(), "b");
  history.Clear();
  EXPECT_EQ(history.Size(), 0u);
  EXPECT_EQ(history.Current(), std::nullopt);
  EXPECT_EQ(history.Previous(), std::nullopt);
  EXPECT_TRUE(history.Search("a").empty());
  // History is usable again after clear.
  history.Add("fresh");
  EXPECT_EQ(history.Size(), 1u);
  EXPECT_EQ(history.Previous(), "fresh");
}

TEST(CommandHistory, PersistenceMockRecordsAcceptedCommands) {
  CommandHistory history(10);
  auto store = std::make_unique<RecordingStore>();
  RecordingStore* raw = store.get();
  history.SetPersistentStore(std::move(store));

  history.Add("ls");
  history.Add("git status");
  // Blank and consecutive-duplicate input must not reach the store.
  history.Add("");
  history.Add("git status");

  const std::vector<std::string> persisted = raw->persisted();
  const std::vector<std::string> expected = {"ls", "git status"};
  EXPECT_EQ(persisted, expected);
}

TEST(CommandHistory, PersistenceFailureDoesNotCorruptHistory) {
  CommandHistory history(10);
  history.SetPersistentStore(std::make_unique<ThrowingStore>());

  // None of these may throw or lose entries even though every Persist throws.
  history.Add("one");
  history.Add("two");
  history.Add("three");

  ASSERT_EQ(history.Size(), 3u);
  EXPECT_EQ(history.Previous(), "three");
  EXPECT_EQ(history.Previous(), "two");
  EXPECT_EQ(history.Previous(), "one");
  EXPECT_TRUE(history.Search("o").size() == 2u);  // "one" and "two" still present.
}

// A policy whose ShouldPersist always throws, to prove a throwing user-supplied
// policy also cannot corrupt the in-memory history.
class ThrowingPolicy : public CommandPersistencePolicy {
 public:
  bool ShouldPersist(std::string_view) const override {
    throw std::runtime_error("policy failure");
  }
};

TEST(CommandHistory, PolicyFailureDoesNotCorruptHistoryOrEscape) {
  CommandHistory history(10);
  auto store = std::make_unique<RecordingStore>();
  RecordingStore* raw = store.get();
  history.SetPersistentStore(std::move(store));
  history.SetPersistencePolicy(std::make_unique<ThrowingPolicy>());

  // ShouldPersist throws for every command; the add must still succeed in
  // memory and the exception must not escape.
  history.Add("one");
  history.Add("two");
  ASSERT_EQ(history.Size(), 2u);
  EXPECT_EQ(history.Previous(), "two");
  EXPECT_EQ(history.Previous(), "one");
  // Nothing was ultimately persisted.
  EXPECT_TRUE(raw->persisted().empty());
}

TEST(CommandHistory, SensitiveCommandNotPersistedButStillStored) {
  CommandHistory history(10);
  auto store = std::make_unique<RecordingStore>();
  RecordingStore* raw = store.get();
  history.SetPersistentStore(std::move(store));
  history.SetPersistencePolicy(
      std::make_unique<SensitiveCommandPolicy>(std::vector<std::string>{"rm -rf /", "secret"}));

  history.Add("ls");
  history.Add("rm -rf /");  // Sensitive: suppressed from persistence.
  history.Add("git status");

  // The sensitive command is still in the in-memory history.
  ASSERT_EQ(history.Size(), 3u);
  EXPECT_EQ(history.Previous(), "git status");
  EXPECT_EQ(history.Previous(), "rm -rf /");
  EXPECT_EQ(history.Previous(), "ls");

  // ... but it was never forwarded to the store.
  const std::vector<std::string> persisted = raw->persisted();
  const std::vector<std::string> expected = {"ls", "git status"};
  EXPECT_EQ(persisted, expected);
}

TEST(CommandHistory, SensitivePolicyUsesExactMatchNotPrefix) {
  CommandHistory history(10);
  auto store = std::make_unique<RecordingStore>();
  RecordingStore* raw = store.get();
  history.SetPersistentStore(std::move(store));
  history.SetPersistencePolicy(
      std::make_unique<SensitiveCommandPolicy>(std::vector<std::string>{"secret"}));

  // "secret" is blocked; "secretive" and "my secret" are not (no trimming,
  // no prefix/substring matching).
  history.Add("secret");
  history.Add("secretive");
  history.Add("my secret");

  const std::vector<std::string> persisted = raw->persisted();
  const std::vector<std::string> expected = {"secretive", "my secret"};
  EXPECT_EQ(persisted, expected);
}

TEST(CommandHistory, SensitivePolicyWithoutStoreHasNoEffect) {
  CommandHistory history(10);
  history.SetPersistencePolicy(
      std::make_unique<SensitiveCommandPolicy>(std::vector<std::string>{"secret"}));
  history.Add("secret");
  // Nothing to persist; the command is stored normally.
  ASSERT_EQ(history.Size(), 1u);
  EXPECT_EQ(history.Previous(), "secret");
}

TEST(CommandHistory, DefaultPolicyPersistsEverything) {
  CommandHistory history(10);
  auto store = std::make_unique<RecordingStore>();
  RecordingStore* raw = store.get();
  history.SetPersistentStore(std::move(store));
  // No policy set: default behavior persists every accepted command.
  history.Add("alpha");
  history.Add("beta");
  const std::vector<std::string> persisted = raw->persisted();
  const std::vector<std::string> expected = {"alpha", "beta"};
  EXPECT_EQ(persisted, expected);
}

TEST(CommandHistory, AlwaysPersistPolicyPersistsEverything) {
  CommandHistory history(10);
  auto store = std::make_unique<RecordingStore>();
  RecordingStore* raw = store.get();
  history.SetPersistentStore(std::move(store));
  history.SetPersistencePolicy(std::make_unique<AlwaysPersistPolicy>());
  history.Add("x");
  const std::vector<std::string> persisted = raw->persisted();
  const std::vector<std::string> expected = {"x"};
  EXPECT_EQ(persisted, expected);
}

TEST(CommandHistory, DetachingStoreStopsPersistence) {
  CommandHistory history(10);
  auto store = std::make_unique<RecordingStore>();
  RecordingStore* raw = store.get();
  history.SetPersistentStore(std::move(store));
  history.SetPersistencePolicy(
      std::make_unique<SensitiveCommandPolicy>(std::vector<std::string>{"topsecret"}));

  history.Add("topsecret");  // Sensitive: never persisted.
  history.Add("ok");
  EXPECT_EQ(raw->persisted().size(), 1u);  // Only "ok" reached the store.

  // Snapshot what was persisted *before* detaching: "topsecret" must not be
  // present. (Reading raw after SetPersistentStore(nullptr) would be a
  // use-after-free because the history destroys the store on detach.)
  const std::vector<std::string> before = raw->persisted();

  history.SetPersistentStore(nullptr);  // Store is destroyed here.

  // After detach, new commands no longer reach any store and, crucially, do
  // not corrupt the in-memory history.
  history.Add("more");
  EXPECT_EQ(history.Size(), 3u);
  EXPECT_EQ(history.Previous(), "more");
  EXPECT_EQ(history.Previous(), "ok");

  const std::vector<std::string> expected = {"ok"};
  EXPECT_EQ(before, expected);
}

TEST(CommandHistory, RetainedCommandsOwnTheirMemory) {
  CommandHistory history(10);
  {
    // Build commands from a short-lived source that dies before navigation.
    std::string scratch = "dead-source-command";
    history.Add(scratch);
    std::string_view borrowed = scratch;
    history.Add(std::string{borrowed.substr(0, 4)});
    scratch.clear();
  }
  ASSERT_EQ(history.Size(), 2u);
  // The retained strings must still hold their full contents after the
  // originating storage went out of scope.
  EXPECT_EQ(history.Previous(), "dead");
  EXPECT_EQ(history.Previous(), "dead-source-command");

  // The same holds after a Clear: no dangling reference can be observed.
  history.Clear();
  EXPECT_EQ(history.Size(), 0u);
}

TEST(CommandHistory, MoveConstructsAndMoves) {
  auto history = CommandHistory(10);
  history.Add("a");
  history.Add("b");

  CommandHistory moved(std::move(history));
  EXPECT_EQ(moved.Size(), 2u);
  EXPECT_EQ(moved.Previous(), "b");
  EXPECT_EQ(moved.Previous(), "a");
}

}  // namespace
}  // namespace editor
}  // namespace terminal_ui_kit
