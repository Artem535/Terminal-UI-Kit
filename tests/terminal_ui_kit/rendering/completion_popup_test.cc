#include "terminal_ui_kit/components/completion_popup.h"

#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/event.hpp>

#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

using ResultFn = std::function<void(CompletionResult)>;

std::vector<CompletionItem> Items() {
  return {
      {"printf", "printf", "Print formatted output", "IO", CompletionKind::kFunction, {}, {}},
      {"fstat", "fstat", "Status of a file", "IO", CompletionKind::kFunction, {}, {}},
      {"int", "int", "Signed integer type", "Type", CompletionKind::kType, {}, {}},
  };
}

std::shared_ptr<SyncCompletionProvider> SyncProvider(std::vector<CompletionItem> items) {
  return std::make_shared<SyncCompletionProvider>(
      [items = std::move(items)](const CompletionContext&) { return items; });
}

// Deterministic single-threaded scheduler for the async provider: work is
// queued and only runs when run_all() is called.
class ManualQueue {
 public:
  void schedule(std::function<void()> fn) { queue_.push_back(std::move(fn)); }
  std::size_t run_all() {
    std::size_t count = queue_.size();
    while (!queue_.empty()) {
      std::function<void()> fn = std::move(queue_.front());
      queue_.pop_front();
      fn();
    }
    return count;
  }
  std::size_t pending() const { return queue_.size(); }

 private:
  std::deque<std::function<void()>> queue_;
};

std::shared_ptr<AsyncCompletionProvider> AsyncProvider(ManualQueue& queue,
                                                       std::vector<CompletionItem> items) {
  return std::make_shared<AsyncCompletionProvider>(
      [items = std::move(items)](const CompletionContext&) { return items; },
      [&queue](std::function<void()> fn) { queue.schedule(std::move(fn)); });
}

std::string StripAnsi(const std::string& in) {
  std::string out;
  for (std::size_t i = 0; i < in.size();) {
    if (in[i] == '\x1b') {
      while (i < in.size() && in[i] != 'm') {
        ++i;
      }
      if (i < in.size()) {
        ++i;
      }
      continue;
    }
    out += in[i];
    ++i;
  }
  return out;
}

TEST(FuzzyMatch, MatchesCaseInsensitiveSubsequence) {
  EXPECT_TRUE(fuzzy_match("ftx", "ftxui").matched);
  EXPECT_TRUE(fuzzy_match("FTX", "ftxui").matched);
  EXPECT_TRUE(fuzzy_match("fui", "ftxui").matched);
  EXPECT_FALSE(fuzzy_match("zzy", "ftxui").matched);
  EXPECT_FALSE(fuzzy_match("ftx", "").matched);
  EXPECT_TRUE(fuzzy_match("", "anything").matched);
}

TEST(FuzzyMatch, PrefersEarlierStartAndConsecutiveRuns) {
  const int orange = fuzzy_match("or", "orange").score;
  const int actor = fuzzy_match("or", "actor").score;
  EXPECT_GT(orange, actor);
  const int consecutive = fuzzy_match("ft", "ft").score;
  const int sparse = fuzzy_match("ft", "filler filler t").score;
  EXPECT_GT(consecutive, sparse);
}

TEST(CompletionPopup, SyncProviderDeliversResults) {
  CompletionPopupModel model(SyncProvider(Items()));
  model.set_max_visible_rows(5);
  model.set_input("p", 1);
  EXPECT_EQ(model.state(), CompletionState::kResults);
  ASSERT_EQ(model.items().size(), 1U);
  EXPECT_EQ(model.items()[0].label, "printf");
}

TEST(CompletionPopup, AsyncProviderLoadingThenResults) {
  ManualQueue queue;
  CompletionPopupModel model(AsyncProvider(queue, Items()));
  model.set_max_visible_rows(5);
  model.set_input("p", 1);
  EXPECT_EQ(model.state(), CompletionState::kLoading);
  EXPECT_TRUE(model.items().empty());
  const std::size_t ran = queue.run_all();
  ASSERT_EQ(ran, 1U);
  EXPECT_EQ(model.state(), CompletionState::kResults);
  ASSERT_EQ(model.items().size(), 1U);
}

TEST(CompletionPopup, NoResultsState) {
  CompletionPopupModel model(SyncProvider(Items()));
  model.set_input("zzz", 3);
  EXPECT_EQ(model.state(), CompletionState::kNoResults);
  EXPECT_TRUE(model.items().empty());
}

TEST(CompletionPopup, ProviderErrorState) {
  auto provider = std::make_shared<SyncCompletionProvider>(
      [](const CompletionContext&) -> std::vector<CompletionItem> {
        throw std::runtime_error("boom");
      });
  CompletionPopupModel model(provider);
  model.set_input("x", 1);
  EXPECT_EQ(model.state(), CompletionState::kError);
  EXPECT_EQ(model.error_text(), "boom");
}

TEST(CompletionPopup, FuzzyFilteringSortsAndDropsNonMatches) {
  CompletionPopupModel model(SyncProvider(Items()));
  // "f" matches printf (trailing f) and fstat (leading f); fstat's earlier
  // start ranks first. "int" and other letters are dropped.
  model.set_input("f", 1);
  ASSERT_EQ(model.state(), CompletionState::kResults);
  const std::vector<CompletionItem> items = model.items();
  ASSERT_EQ(items.size(), 2U);
  EXPECT_EQ(items[0].label, "fstat");
  EXPECT_EQ(items[1].label, "printf");
}

TEST(CompletionPopup, KeyboardNavigationMovesSelection) {
  CompletionPopupModel model(SyncProvider(Items()));
  model.set_input("", 0);
  // Empty query hides; type a wide query to match everything.
  model.set_input("f", 1);
  const std::size_t count = model.items().size();
  ASSERT_GE(count, 2U);
  EXPECT_EQ(model.selected(), 0U);
  EXPECT_TRUE(model.component()->OnEvent(ftxui::Event::ArrowDown));
  EXPECT_EQ(model.selected(), 1U);
  EXPECT_TRUE(model.component()->OnEvent(ftxui::Event::ArrowUp));
  EXPECT_EQ(model.selected(), 0U);
}

TEST(CompletionPopup, AcceptedByEnterAppliesReplacementAcrossToken) {
  std::string applied_text;
  int applied_cursor = -1;
  CompletionPopupModel model(SyncProvider(Items()));
  model.set_on_accept([&](const CompletionItem&, const std::string& text, int cursor) {
    applied_text = text;
    applied_cursor = cursor;
  });
  model.set_input("pri", 3);
  EXPECT_TRUE(model.component()->OnEvent(ftxui::Event::Return));
  EXPECT_EQ(applied_text, "printf");
  EXPECT_EQ(applied_cursor, 6);
  EXPECT_EQ(model.state(), CompletionState::kHidden);
}

TEST(CompletionPopup, TabAcceptsSelection) {
  bool accepted = false;
  CompletionPopupModel model(SyncProvider(Items()));
  model.set_on_accept([&](const CompletionItem&, const std::string&, int) { accepted = true; });
  model.set_input("ft", 2);
  EXPECT_TRUE(model.component()->OnEvent(ftxui::Event::Tab));
  EXPECT_TRUE(accepted);
}

TEST(CompletionPopup, EscapeCancelsAndHides) {
  CompletionPopupModel model(SyncProvider(Items()));
  model.set_input("f", 1);
  EXPECT_NE(model.state(), CompletionState::kHidden);
  EXPECT_TRUE(model.component()->OnEvent(ftxui::Event::Escape));
  EXPECT_EQ(model.state(), CompletionState::kHidden);
}

TEST(CompletionPopup, ExplicitReplacementRangeIsApplied) {
  std::vector<CompletionItem> items = {{
      "foobar",
      "foobar",
      "",
      "",
      CompletionKind::kFunction,
      /*range*/ {4, 7},
      /*meta*/ {},
  }};
  std::string applied;
  CompletionPopupModel model(SyncProvider(items));
  model.set_on_accept([&](const CompletionItem&, const std::string& text, int) { applied = text; });
  // Line "pre foo": the token "foo" occupies bytes [4,7); item replaces that
  // explicit range with "foobar".
  model.set_input("pre foo", 7);
  ASSERT_EQ(model.state(), CompletionState::kResults);
  EXPECT_TRUE(model.component()->OnEvent(ftxui::Event::Return));
  EXPECT_EQ(applied, "pre foobar");
}

TEST(CompletionPopup, InvalidReplacementRangeFallsBackToToken) {
  std::vector<CompletionItem> items = {{
      "foobar",
      "foobar",
      "",
      "",
      CompletionKind::kUnknown,
      /*range*/ {9, 2},  // begin > end, out of line bounds
      /*meta*/ {},
  }};
  std::string applied;
  CompletionPopupModel model(SyncProvider(items));
  model.set_on_accept([&](const CompletionItem&, const std::string& text, int) { applied = text; });
  model.set_input("pre foo", 7);
  ASSERT_EQ(model.state(), CompletionState::kResults);
  EXPECT_TRUE(model.component()->OnEvent(ftxui::Event::Return));
  // The invalid item range is ignored; the token range [4,7) is replaced.
  EXPECT_EQ(applied, "pre foobar");
}

TEST(CompletionPopup, StaleResponseDoesNotReplaceNewerResults) {
  ManualQueue queue;
  // The provider answers with the query echoed in the label, so the two
  // in-flight requests would produce different results if both were honoured.
  std::shared_ptr<AsyncCompletionProvider> provider = std::make_shared<AsyncCompletionProvider>(
      [](const CompletionContext& context) -> std::vector<CompletionItem> {
        return {{context.query + "Result",
                 context.query + "Result",
                 "",
                 "",
                 CompletionKind::kUnknown,
                 {},
                 {}}};
      },
      [&queue](std::function<void()> fn) { queue.schedule(std::move(fn)); });
  CompletionPopupModel model(provider);
  model.set_input("a", 1);   // Request 1 (query "a", in flight).
  model.set_input("ab", 2);  // Request 2 (query "ab") supersedes request 1.
  ASSERT_EQ(queue.pending(), 2U);
  queue.run_all();
  // Only the newest generation may present results: the "ab" response, not "a".
  ASSERT_EQ(model.state(), CompletionState::kResults);
  ASSERT_EQ(model.items().size(), 1U);
  EXPECT_EQ(model.items()[0].label, "abResult");
}

TEST(CompletionPopup, DuplicateLabelsAreDistinctAndNavigable) {
  std::vector<CompletionItem> items = {
      {"apple", "apple", "Fruit", "A", CompletionKind::kUnknown, {}, {}},
      {"apple", "apple", "Company", "B", CompletionKind::kUnknown, {}, {}},
  };
  CompletionPopupModel model(SyncProvider(items));
  model.set_input("ap", 2);
  ASSERT_EQ(model.state(), CompletionState::kResults);
  ASSERT_EQ(model.items().size(), 2U);
  model.select_index(1);
  EXPECT_EQ(model.selected(), 1U);
}

TEST(CompletionPopup, CategoriesRenderInWindow) {
  CompletionPopupModel model(SyncProvider(Items()));
  model.set_max_visible_rows(10);
  model.set_anchor_row(2);
  model.set_viewport_height(20);
  model.set_input("i", 1);
  const std::string text =
      StripAnsi(test_support::render_to_text(model.component()->Render(), 40, 14));
  EXPECT_NE(text.find("int"), std::string::npos);
  EXPECT_NE(text.find("Type"), std::string::npos);
}

TEST(Placement, ChoosesBelowWhenRoomBelow) {
  EXPECT_EQ(choose_placement(3, 1, 10), PopupPlacement::kBelow);
}

TEST(Placement, ChoosesAboveWhenNoRoomBelow) {
  EXPECT_EQ(choose_placement(3, 8, 10), PopupPlacement::kAbove);
}

TEST(Placement, NarrowViewportTakesSideWithMoreSpace) {
  // 10 rows, caret at 8, need 6: below=1, above=8 -> kAbove, clamped to 8.
  EXPECT_EQ(choose_placement(6, 8, 10), PopupPlacement::kAbove);
  EXPECT_EQ(clamp_visible_rows(6, 8, 10), 6);   // clamped to needed (above has room)
  EXPECT_EQ(clamp_visible_rows(6, 2, 10), 6);   // plenty below
  EXPECT_EQ(clamp_visible_rows(20, 0, 10), 9);  // clamped by available below
}

TEST(CompletionPopup, AbovePlacementIsConfigured) {
  CompletionPopupModel model(SyncProvider(Items()));
  model.set_max_visible_rows(3);
  model.set_anchor_row(8);
  model.set_viewport_height(10);
  model.recompute_placement();
  EXPECT_EQ(model.placement(), PopupPlacement::kAbove);
}

TEST(CompletionPopup, BelowPlacementIsConfigured) {
  CompletionPopupModel model(SyncProvider(Items()));
  model.set_max_visible_rows(3);
  model.set_anchor_row(1);
  model.set_viewport_height(10);
  model.recompute_placement();
  EXPECT_EQ(model.placement(), PopupPlacement::kBelow);
}

TEST(CompletionPopup, ResizeRecomputesPlacement) {
  CompletionPopupModel model(SyncProvider(Items()));
  model.set_max_visible_rows(3);
  model.set_anchor_row(8);
  model.set_viewport_height(40);
  model.recompute_placement();
  EXPECT_EQ(model.placement(), PopupPlacement::kBelow);
  model.set_viewport_height(10);
  model.recompute_placement();
  EXPECT_EQ(model.placement(), PopupPlacement::kAbove);
}

TEST(CompletionPopup, RendersWindowInTopRowsWhenAbove) {
  CompletionPopupModel model(SyncProvider(Items()));
  model.set_max_visible_rows(3);
  model.set_anchor_row(8);
  model.set_viewport_height(10);
  model.set_input("f", 1);
  model.recompute_placement();
  ASSERT_EQ(model.placement(), PopupPlacement::kAbove);
  const ftxui::Screen screen = test_support::render_to_screen(model.component()->Render(), 40, 10);
  // Window spans rows 6..8 (top row 8-3+1). Rows 0..5 should be blank (space).
  EXPECT_EQ(StripAnsi(screen.PixelAt(0, 5).character), " ");
  EXPECT_NE(StripAnsi(screen.PixelAt(0, 6).character), " ");
}

TEST(CompletionPopup, RendersWindowInBottomRowsWhenBelow) {
  CompletionPopupModel model(SyncProvider(Items()));
  model.set_max_visible_rows(3);
  model.set_anchor_row(2);
  model.set_viewport_height(10);
  model.set_input("f", 1);
  model.recompute_placement();
  ASSERT_EQ(model.placement(), PopupPlacement::kBelow);
  const ftxui::Screen screen = test_support::render_to_screen(model.component()->Render(), 40, 10);
  // Window starts below the caret at row 3.
  EXPECT_EQ(StripAnsi(screen.PixelAt(0, 2).character), " ");
  EXPECT_NE(StripAnsi(screen.PixelAt(0, 3).character), " ");
}

TEST(CompletionPopup, EmptyQueryHides) {
  CompletionPopupModel model(SyncProvider(Items()));
  model.set_input("", 0);
  EXPECT_EQ(model.state(), CompletionState::kHidden);
  model.set_input(" ", 1);
  EXPECT_EQ(model.state(), CompletionState::kHidden);
}

TEST(CompletionPopup, DestructionWithPendingRequestIsSafe) {
  ManualQueue queue;
  auto provider = std::make_shared<AsyncCompletionProvider>(
      [](const CompletionContext&) -> std::vector<CompletionItem> {
        return {{"late", "late", "", "", CompletionKind::kUnknown, {}, {}}};
      },
      [&queue](std::function<void()> fn) { queue.schedule(std::move(fn)); });
  {
    CompletionPopupModel model(provider);
    model.set_input("x", 1);
    ASSERT_EQ(queue.pending(), 1U);
    // Model + component go out of scope here (destroyed) with work still queued.
  }
  // Draining after destruction must not touch freed memory.
  EXPECT_NO_FATAL_FAILURE(queue.run_all());
}

}  // namespace
}  // namespace terminal_ui_kit