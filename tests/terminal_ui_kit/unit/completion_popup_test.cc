#include "terminal_ui_kit/components/completion_popup.h"

#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

using State = CompletionPopup::State;

CompletionItem MakeItem(std::string label, std::string category = {}, std::string description = {},
                        std::string insert_text = {}) {
  CompletionItem item;
  item.label = std::move(label);
  item.category = std::move(category);
  item.description = std::move(description);
  item.insert_text = std::move(insert_text);
  return item;
}

// Deterministic async provider: records every request and lets the test fire
// responses when it chooses. The popup drives generation handling.
class ManualAsyncProvider : public ICompletionProvider {
 public:
  struct Request {
    std::uint64_t generation;
    CompletionContext context;
    std::function<void(std::uint64_t, CompletionResult)> deliver;
  };

  void complete(const CompletionContext& context, std::uint64_t generation,
                const std::function<void(std::uint64_t, CompletionResult)>& deliver) override {
    requests.push_back({generation, context, deliver});
  }

  void Respond(std::size_t index, std::vector<CompletionItem> items) {
    CompletionResult result;
    result.items = std::move(items);
    requests[index].deliver(requests[index].generation, std::move(result));
  }

  void Fail(std::size_t index, std::string message) {
    CompletionResult result;
    result.error = std::move(message);
    requests[index].deliver(requests[index].generation, std::move(result));
  }

  std::vector<Request> requests;
};

// ---------------------------------------------------------------------------
// Pure logic: fuzzy filtering
// ---------------------------------------------------------------------------

TEST(CompletionPopupFuzzy, EmptyQueryMatchesEverything) {
  EXPECT_TRUE(CompletionPopup::FuzzyMatch("", "anything"));
}

TEST(CompletionPopupFuzzy, ExactPrefixMatches) {
  EXPECT_TRUE(CompletionPopup::FuzzyMatch("hel", "hello"));
}

TEST(CompletionPopupFuzzy, SubsequenceMatches) {
  EXPECT_TRUE(CompletionPopup::FuzzyMatch("hlo", "hello"));
  EXPECT_TRUE(CompletionPopup::FuzzyMatch("abc", "aXbYcZ"));
}

TEST(CompletionPopupFuzzy, CaseInsensitive) {
  EXPECT_TRUE(CompletionPopup::FuzzyMatch("HEL", "hello"));
  EXPECT_TRUE(CompletionPopup::FuzzyMatch("hel", "HELLO"));
}

TEST(CompletionPopupFuzzy, NonSubsequenceDoesNotMatch) {
  EXPECT_FALSE(CompletionPopup::FuzzyMatch("xyz", "hello"));
  EXPECT_FALSE(CompletionPopup::FuzzyMatch("hllx", "hello"));
  EXPECT_FALSE(CompletionPopup::FuzzyMatch("lengthierthan", "abc"));
}

// ---------------------------------------------------------------------------
// Pure logic: replacement ranges
// ---------------------------------------------------------------------------

TEST(CompletionPopupReplacement, NoRangeReplacesTypedQueryAtCursor) {
  CompletionItem item = MakeItem("request", "core", "", "fetch_request");
  const std::string out = CompletionPopup::ApplyReplacement("print requ", 10, "requ", item);
  EXPECT_EQ(out, "print fetch_request");
}

TEST(CompletionPopupReplacement, NoRangeWithInsertTextFallsBackToLabel) {
  CompletionItem item = MakeItem("for_each");
  const std::string out = CompletionPopup::ApplyReplacement("x for", 5, "for", item);
  EXPECT_EQ(out, "x for_each");
}

TEST(CompletionPopupReplacement, ExplicitRangeReplacesOnlyThatRegion) {
  CompletionItem item = MakeItem("b", "", "", "BB");
  item.replacement_range = std::make_pair<std::size_t, std::size_t>(2, 5);
  const std::string out = CompletionPopup::ApplyReplacement("aabbbxx", 7, "", item);
  EXPECT_EQ(out, "aaBBxx");
}

TEST(CompletionPopupReplacement, InvalidReversedRangeLeavesBufferUntouched) {
  CompletionItem item = MakeItem("b", "", "", "BB");
  item.replacement_range = std::make_pair<std::size_t, std::size_t>(5, 2);
  const std::string out = CompletionPopup::ApplyReplacement("aabbcc", 6, "a", item);
  EXPECT_EQ(out, "aabbcc");
}

TEST(CompletionPopupReplacement, InvalidOutOfBoundsRangeLeavesBufferUntouched) {
  CompletionItem item = MakeItem("b", "", "", "BB");
  item.replacement_range = std::make_pair<std::size_t, std::size_t>(4, 99);
  const std::string out = CompletionPopup::ApplyReplacement("abcd", 4, "a", item);
  EXPECT_EQ(out, "abcd");
}

TEST(CompletionPopupReplacement, InsertsAtEndWhenCursorAtBufferEnd) {
  CompletionItem item = MakeItem("query", "", "", "");
  const std::string out = CompletionPopup::ApplyReplacement("prefix q", 8, "q", item);
  EXPECT_EQ(out, "prefix query");
}

// ---------------------------------------------------------------------------
// Pure logic: viewport-aware placement
// ---------------------------------------------------------------------------

TEST(CompletionPopupPlacement, BelowWhenItFitsBelow) {
  EXPECT_EQ(CompletionPopup::decide_placement(5, 0, 3), CompletionPopup::Placement::kBelow);
}

TEST(CompletionPopupPlacement, AboveWhenItDoesNotFitBelowButFitsAbove) {
  EXPECT_EQ(CompletionPopup::decide_placement(1, 5, 3), CompletionPopup::Placement::kAbove);
}

TEST(CompletionPopupPlacement, PrefersSideWithMoreRoomWhenNeitherFits) {
  EXPECT_EQ(CompletionPopup::decide_placement(1, 4, 10), CompletionPopup::Placement::kAbove);
  EXPECT_EQ(CompletionPopup::decide_placement(4, 1, 10), CompletionPopup::Placement::kBelow);
}

TEST(CompletionPopupPlacement, TiesResolveToBelow) {
  EXPECT_EQ(CompletionPopup::decide_placement(2, 2, 10), CompletionPopup::Placement::kBelow);
}

// ---------------------------------------------------------------------------
// Synchronous provider + state transitions
// ---------------------------------------------------------------------------

TEST(CompletionPopupSync, ProducesFilteredResults) {
  ManualAsyncProvider provider;
  CompletionPopupOptions options;
  options.provider = std::shared_ptr<ICompletionProvider>(&provider, [](ICompletionProvider*) {});
  CompletionPopup popup(nullptr, options);
  popup.set_query("fi", 2);
  EXPECT_EQ(popup.state(), State::kLoading);
  ASSERT_EQ(provider.requests.size(), 1U);
  provider.Respond(0, {MakeItem("file"), MakeItem("find"), MakeItem("other")});
  EXPECT_EQ(popup.state(), State::kResults);
  const auto& items = popup.items();
  ASSERT_EQ(items.size(), 2U);
  EXPECT_EQ(items[0].label, "file");
  EXPECT_EQ(items[1].label, "find");
}

TEST(CompletionPopupSync, NoResultsState) {
  ManualAsyncProvider provider;
  CompletionPopupOptions options;
  options.provider = std::shared_ptr<ICompletionProvider>(&provider, [](ICompletionProvider*) {});
  CompletionPopup popup(nullptr, options);
  popup.set_query("zz", 2);
  provider.Respond(0, {MakeItem("alpha")});
  EXPECT_EQ(popup.state(), State::kNoResults);
  EXPECT_TRUE(popup.visible());
}

TEST(CompletionPopupSync, ProviderErrorState) {
  ManualAsyncProvider provider;
  CompletionPopupOptions options;
  options.provider = std::shared_ptr<ICompletionProvider>(&provider, [](ICompletionProvider*) {});
  CompletionPopup popup(nullptr, options);
  popup.set_query("ab", 2);
  provider.Fail(0, "backend down");
  EXPECT_EQ(popup.state(), State::kError);
  EXPECT_FALSE(popup.accept_selected());
}

TEST(CompletionPopupSync, EmptyQueryStaysHiddenAndIssuesNoRequest) {
  ManualAsyncProvider provider;
  CompletionPopupOptions options;
  options.provider = std::shared_ptr<ICompletionProvider>(&provider, [](ICompletionProvider*) {});
  CompletionPopup popup(nullptr, options);
  popup.set_query("", 0);
  EXPECT_EQ(popup.state(), State::kHidden);
  EXPECT_TRUE(provider.requests.empty());
}

TEST(CompletionPopupSync, ShortQueryHiddenRespectsMinLength) {
  ManualAsyncProvider provider;
  CompletionPopupOptions options;
  options.provider = std::shared_ptr<ICompletionProvider>(&provider, [](ICompletionProvider*) {});
  options.min_query_length = 2;
  CompletionPopup popup(nullptr, options);
  popup.set_query("a", 1);
  EXPECT_EQ(popup.state(), State::kHidden);
  EXPECT_TRUE(provider.requests.empty());
  popup.set_query("ab", 2);
  EXPECT_EQ(popup.state(), State::kLoading);
  ASSERT_EQ(provider.requests.size(), 1U);
}

// ---------------------------------------------------------------------------
// Async: loading, stale, cancellation, destruction
// ---------------------------------------------------------------------------

TEST(CompletionPopupAsync, LoadingStateBeforeResponse) {
  ManualAsyncProvider provider;
  CompletionPopupOptions options;
  options.provider = std::shared_ptr<ICompletionProvider>(&provider, [](ICompletionProvider*) {});
  CompletionPopup popup(nullptr, options);
  popup.set_query("ca", 2);
  EXPECT_EQ(popup.state(), State::kLoading);
  EXPECT_TRUE(popup.visible());
  EXPECT_TRUE(popup.items().empty());
}

TEST(CompletionPopupAsync, StaleResponseDoesNotReplaceNewerQuery) {
  ManualAsyncProvider provider;
  CompletionPopupOptions options;
  options.provider = std::shared_ptr<ICompletionProvider>(&provider, [](ICompletionProvider*) {});
  CompletionPopup popup(nullptr, options);
  popup.set_query("ca", 2);                     // request 0
  popup.set_query("cat", 3);                    // request 1 supersedes request 0
  provider.Respond(1, {MakeItem("category")});  // latest response lands
  EXPECT_EQ(popup.state(), State::kResults);
  EXPECT_EQ(popup.items().size(), 1U);
  provider.Respond(0, {MakeItem("CAR", "stale")});  // stale response is dropped
  EXPECT_EQ(popup.state(), State::kResults);
  ASSERT_EQ(popup.items().size(), 1U);
  EXPECT_EQ(popup.items()[0].label, "category");
}

TEST(CompletionPopupAsync, NewerQueryInvalidatesPendingRequest) {
  ManualAsyncProvider provider;
  CompletionPopupOptions options;
  options.provider = std::shared_ptr<ICompletionProvider>(&provider, [](ICompletionProvider*) {});
  CompletionPopup popup(nullptr, options);
  popup.set_query("a", 1);
  popup.set_query("ab", 2);  // supersede the first
  EXPECT_EQ(popup.state(), State::kLoading);
  provider.Respond(0, {MakeItem("alpha")});  // firing the older request does nothing
  EXPECT_EQ(popup.state(), State::kLoading);
  EXPECT_TRUE(popup.items().empty());
  provider.Respond(1, {MakeItem("able")});
  EXPECT_EQ(popup.state(), State::kResults);
  EXPECT_EQ(popup.items().size(), 1U);
  EXPECT_EQ(popup.items()[0].label, "able");
}

TEST(CompletionPopupAsync, HiddenPopupIgnoresPendingDelivery) {
  ManualAsyncProvider provider;
  CompletionPopupOptions options;
  options.provider = std::shared_ptr<ICompletionProvider>(&provider, [](ICompletionProvider*) {});
  CompletionPopup popup(nullptr, options);
  popup.set_query("ab", 2);
  popup.hide();
  provider.Respond(0, {MakeItem("able")});
  EXPECT_EQ(popup.state(), State::kHidden);
  EXPECT_FALSE(popup.visible());
}

TEST(CompletionPopupAsync, LateCallbackAfterDestructionIsSafe) {
  ManualAsyncProvider provider;
  {
    CompletionPopupOptions options;
    options.provider = std::shared_ptr<ICompletionProvider>(&provider, [](ICompletionProvider*) {});
    CompletionPopup popup(nullptr, options);
    popup.set_query("ab", 2);
    ASSERT_EQ(provider.requests.size(), 1U);
    // popup goes out of scope with a request still pending.
  }
  // Firing the captured callback after destruction must not touch freed state.
  provider.Respond(0, {MakeItem("able")});
  SUCCEED();
}

// ---------------------------------------------------------------------------
// SyncCompletionProvider adapter
// ---------------------------------------------------------------------------

TEST(SyncCompletionProvider, InvokesDeliverSynchronously) {
  auto provider = std::make_shared<SyncCompletionProvider>([](const CompletionContext& context) {
    return std::vector<CompletionItem>{MakeItem("echo:" + context.query)};
  });
  CompletionPopupOptions options;
  options.provider = provider;
  CompletionPopup popup(nullptr, options);
  popup.set_query("x", 1);
  EXPECT_EQ(popup.state(), State::kResults);
  ASSERT_EQ(popup.items().size(), 1U);
  EXPECT_EQ(popup.items()[0].label, "echo:x");
}

TEST(SyncCompletionProvider, ThrowingProviderBecomesErrorState) {
  auto provider = std::make_shared<SyncCompletionProvider>([](const CompletionContext&) {
    throw std::runtime_error("boom");
    return std::vector<CompletionItem>{};
  });
  CompletionPopupOptions options;
  options.provider = provider;
  CompletionPopup popup(nullptr, options);
  popup.set_query("x", 1);
  EXPECT_EQ(popup.state(), State::kError);
}

// ---------------------------------------------------------------------------
// Selection, acceptance, duplicate labels, metadata
// ---------------------------------------------------------------------------

TEST(CompletionPopupSelection, NavigatesAndClampsWithinRange) {
  ManualAsyncProvider provider;
  CompletionPopupOptions options;
  options.provider = std::shared_ptr<ICompletionProvider>(&provider, [](ICompletionProvider*) {});
  CompletionPopup popup(nullptr, options);
  popup.set_query("a", 1);
  provider.Respond(0, {MakeItem("alpha"), MakeItem("able"), MakeItem("actor")});
  EXPECT_EQ(popup.selected_index(), 0U);
  popup.move_selection(1);
  EXPECT_EQ(popup.selected_index(), 1U);
  popup.move_selection(1);
  EXPECT_EQ(popup.selected_index(), 2U);
  popup.move_selection(1);  // clamps at end
  EXPECT_EQ(popup.selected_index(), 2U);
  popup.move_selection(-3);  // clamps at start
  EXPECT_EQ(popup.selected_index(), 0U);
}

TEST(CompletionPopupSelection, AcceptInvokesCallbackWithSelectedItem) {
  ManualAsyncProvider provider;
  std::string accepted;
  CompletionPopupOptions options;
  options.provider = std::shared_ptr<ICompletionProvider>(&provider, [](ICompletionProvider*) {});
  options.on_accept = [&accepted](const CompletionItem& item) { accepted = item.label; };
  CompletionPopup popup(nullptr, options);
  popup.set_query("s", 1);
  provider.Respond(0, {MakeItem("second"), MakeItem("sort")});
  popup.select_index(1);
  EXPECT_TRUE(popup.accept_selected());
  EXPECT_EQ(accepted, "sort");
  EXPECT_EQ(popup.state(), State::kHidden);
}

TEST(CompletionPopupSelection, AcceptWithNoResultsReturnsFalse) {
  ManualAsyncProvider provider;
  CompletionPopupOptions options;
  options.provider = std::shared_ptr<ICompletionProvider>(&provider, [](ICompletionProvider*) {});
  CompletionPopup popup(nullptr, options);
  popup.set_query("zz", 2);
  provider.Respond(0, {});
  EXPECT_FALSE(popup.accept_selected());
}

TEST(CompletionPopupSelection, DuplicateLabelsAreBothSelectable) {
  ManualAsyncProvider provider;
  std::vector<std::string> accepted;
  CompletionPopupOptions options;
  options.provider = std::shared_ptr<ICompletionProvider>(&provider, [](ICompletionProvider*) {});
  options.on_accept = [&accepted](const CompletionItem& item) { accepted.push_back(item.label); };
  CompletionPopup popup(nullptr, options);
  popup.set_query("set", 3);
  provider.Respond(0, {MakeItem("set", "std"), MakeItem("set", "app")});
  ASSERT_EQ(popup.items().size(), 2U);
  EXPECT_EQ(popup.items()[0].category, "std");
  EXPECT_EQ(popup.items()[1].category, "app");
  popup.select_index(1);
  EXPECT_TRUE(popup.accept_selected());
  ASSERT_EQ(accepted.size(), 1U);
  EXPECT_EQ(accepted[0], "set");
  EXPECT_EQ(popup.items()[1].category, "app");  // still there, distinct rows
}

TEST(CompletionPopupSelection, MetadataIsRetainedOwningly) {
  ManualAsyncProvider provider;
  auto payload = std::make_shared<int>(42);
  CompletionPopupOptions options;
  options.provider = std::shared_ptr<ICompletionProvider>(&provider, [](ICompletionProvider*) {});
  CompletionPopup popup(nullptr, options);
  popup.set_query("a", 1);
  CompletionItem item = MakeItem("alpha");
  item.metadata = payload;
  provider.Respond(0, {item});
  ASSERT_EQ(popup.items().size(), 1U);
  EXPECT_EQ(*std::static_pointer_cast<int>(popup.items()[0].metadata), 42);
}

// ---------------------------------------------------------------------------
// Synchronous adapter end-to-end + fuzzy filtering toggle
// ---------------------------------------------------------------------------

TEST(CompletionPopupFuzzyDisabled, ShowsProviderResultsVerbatim) {
  auto provider = std::make_shared<SyncCompletionProvider>(
      [](const CompletionContext&) { return std::vector<CompletionItem>{MakeItem("abc")}; });
  CompletionPopupOptions options;
  options.provider = provider;
  options.fuzzy_filter = false;
  CompletionPopup popup(nullptr, options);
  popup.set_query("zz", 2);  // would filter out "abc" if fuzzy were enabled
  EXPECT_EQ(popup.state(), State::kResults);
  ASSERT_EQ(popup.items().size(), 1U);
  EXPECT_EQ(popup.items()[0].label, "abc");
}

// ---------------------------------------------------------------------------
// show/toggle recovery and public-API edge cases
// ---------------------------------------------------------------------------

TEST(CompletionPopupLifecycle, ShowRecoversNoResultsByReissuingQuery) {
  ManualAsyncProvider provider;
  CompletionPopupOptions options;
  options.provider = std::shared_ptr<ICompletionProvider>(&provider, [](ICompletionProvider*) {});
  CompletionPopup popup(nullptr, options);
  popup.set_query("zz", 2);
  EXPECT_EQ(popup.state(), State::kLoading);
  ASSERT_EQ(provider.requests.size(), 1U);
  provider.Respond(0, {});  // no results -> kNoResults
  EXPECT_EQ(popup.state(), State::kNoResults);
  const std::size_t requests_before = provider.requests.size();
  popup.show();  // re-runs the query and leaves the no-results state
  EXPECT_GT(provider.requests.size(), requests_before);
  EXPECT_EQ(popup.state(), State::kLoading);
}

TEST(CompletionPopupLifecycle, ShowDoesNotDisturbActiveResults) {
  ManualAsyncProvider provider;
  CompletionPopupOptions options;
  options.provider = std::shared_ptr<ICompletionProvider>(&provider, [](ICompletionProvider*) {});
  CompletionPopup popup(nullptr, options);
  popup.set_query("z", 1);
  provider.Respond(0, {MakeItem("zebra")});
  ASSERT_EQ(popup.state(), State::kResults);
  ASSERT_EQ(provider.requests.size(), 1U);
  popup.show();
  EXPECT_EQ(provider.requests.size(), 1U);  // no re-issue while showing results
  EXPECT_EQ(popup.state(), State::kResults);
}

TEST(CompletionPopupLifecycle, ToggleHidesAndShows) {
  ManualAsyncProvider provider;
  CompletionPopupOptions options;
  options.provider = std::shared_ptr<ICompletionProvider>(&provider, [](ICompletionProvider*) {});
  CompletionPopup popup(nullptr, options);
  popup.set_query("z", 1);
  provider.Respond(0, {MakeItem("zebra")});
  EXPECT_TRUE(popup.visible());
  popup.toggle();
  EXPECT_FALSE(popup.visible());
  EXPECT_EQ(popup.state(), State::kHidden);
  popup.toggle();
  EXPECT_TRUE(popup.visible());
}

TEST(CompletionPopupSelection, MoveSelectionHandlesIntMinWithoutOverflow) {
  ManualAsyncProvider provider;
  CompletionPopupOptions options;
  options.provider = std::shared_ptr<ICompletionProvider>(&provider, [](ICompletionProvider*) {});
  CompletionPopup popup(nullptr, options);
  popup.set_query("z", 1);
  provider.Respond(0, {MakeItem("zebra"), MakeItem("zulu")});
  popup.move_selection(1);  // move to the last element
  EXPECT_EQ(popup.selected_index(), 1U);
  popup.move_selection(std::numeric_limits<int>::min());  // must not overflow
  EXPECT_EQ(popup.selected_index(), 0U);
}

}  // namespace
}  // namespace terminal_ui_kit
