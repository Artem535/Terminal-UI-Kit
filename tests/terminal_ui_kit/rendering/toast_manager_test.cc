#include "terminal_ui_kit/components/toast_manager.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

using std::chrono_literals::operator""ms;
using std::chrono_literals::operator""s;

// A controllable clock so expiry can be driven without sleeping.
class FakeClock : public ToastClock {
 public:
  void Advance(std::chrono::steady_clock::duration delta) { now_ += delta; }
  void Set(std::chrono::steady_clock::time_point value) { now_ = value; }
  std::chrono::steady_clock::time_point now() const override { return now_; }

 private:
  std::chrono::steady_clock::time_point now_{};
};

ToastOptions Make(std::string message, ToastSeverity severity = ToastSeverity::kInfo,
                  std::optional<std::chrono::milliseconds> timeout = 5s,
                  std::optional<ToastAction> action = std::nullopt) {
  return ToastOptions{std::move(message), severity, timeout, std::move(action)};
}

ToastOptions Persistent(std::string message) {
  return Make(std::move(message), ToastSeverity::kInfo, std::nullopt);
}

std::vector<std::string> Messages(const ToastManager& manager) {
  std::vector<std::string> out;
  for (const ToastInfo& toast : manager.visible()) {
    out.push_back(toast.message);
  }
  return out;
}

class ToastManagerTest : public ::testing::Test {
 protected:
  ToastManagerTest() : clock_(std::make_shared<FakeClock>()), manager_(clock_, /*max_visible=*/3) {}

  std::shared_ptr<FakeClock> clock_;
  ToastManager manager_;
};

TEST_F(ToastManagerTest, OneToastIsVisible) {
  manager_.show(Make("hello"));

  ASSERT_EQ(manager_.visible_count(), 1u);
  ASSERT_EQ(manager_.visible().size(), 1u);
  EXPECT_EQ(manager_.visible()[0].message, "hello");
  EXPECT_EQ(manager_.queued_count(), 0u);
}

TEST_F(ToastManagerTest, EachSeverityIsPreserved) {
  manager_.show(Make("i", ToastSeverity::kInfo));
  manager_.show(Make("s", ToastSeverity::kSuccess));
  manager_.show(Make("w", ToastSeverity::kWarning));
  manager_.show(Make("e", ToastSeverity::kError));

  const std::vector<ToastInfo> visible = manager_.visible();
  ASSERT_EQ(visible.size(), 3u);
  EXPECT_EQ(visible[0].severity, ToastSeverity::kInfo);
  EXPECT_EQ(visible[1].severity, ToastSeverity::kSuccess);
  EXPECT_EQ(visible[2].severity, ToastSeverity::kWarning);
}

TEST_F(ToastManagerTest, ShowReturnsStableIds) {
  const std::uint64_t first = manager_.show(Make("a"));
  const std::uint64_t second = manager_.show(Make("b"));

  EXPECT_NE(first, second);
  EXPECT_EQ(manager_.visible()[0].id, first);
  EXPECT_EQ(manager_.visible()[1].id, second);
}

TEST_F(ToastManagerTest, QueuePreservesInsertionOrder) {
  // max_visible = 3 (from fixture). Show 6 toasts.
  for (int i = 1; i <= 6; ++i) {
    manager_.show(Make("toast-" + std::to_string(i)));
  }

  ASSERT_EQ(manager_.visible_count(), 3u);
  ASSERT_EQ(manager_.queued_count(), 3u);
  EXPECT_EQ(Messages(manager_), (std::vector<std::string>{"toast-1", "toast-2", "toast-3"}));

  // Close the oldest visible; the longest-waiting queued toast is promoted.
  manager_.close(manager_.visible()[0].id);
  ASSERT_EQ(manager_.visible_count(), 3u);
  EXPECT_EQ(Messages(manager_), (std::vector<std::string>{"toast-2", "toast-3", "toast-4"}));
}

TEST_F(ToastManagerTest, VisibleCountRespectsLimit) {
  // max_visible = 3; adding 5 leaves 2 queued.
  for (int i = 1; i <= 5; ++i) {
    const std::uint64_t id = manager_.show(Make("t" + std::to_string(i)));
    EXPECT_NE(id, 0u);
  }

  EXPECT_EQ(manager_.visible_count(), 3u);
  EXPECT_EQ(manager_.queued_count(), 2u);
  EXPECT_EQ(manager_.max_visible(), 3u);
}

TEST_F(ToastManagerTest, TimedToastExpires) {
  manager_.show(Make("ephemeral", ToastSeverity::kInfo, 5s));

  manager_.update();  // establish the clock baseline
  ASSERT_EQ(manager_.visible_count(), 1u);
  clock_->Advance(6s);
  manager_.update();

  EXPECT_TRUE(manager_.empty());
}

TEST_F(ToastManagerTest, TimedToastNeverExpiresBeforeItsDeadline) {
  manager_.show(Make("ephemeral", ToastSeverity::kInfo, 5s));

  manager_.update();
  clock_->Advance(4s);
  manager_.update();

  ASSERT_EQ(manager_.visible_count(), 1u);
  EXPECT_EQ(manager_.visible()[0].message, "ephemeral");
}

TEST_F(ToastManagerTest, PersistentToastNeverTimesOut) {
  manager_.show(Persistent("sticky"));

  manager_.update();
  clock_->Advance(3600s);
  manager_.update();

  ASSERT_EQ(manager_.visible_count(), 1u);
  EXPECT_EQ(manager_.visible()[0].persistent, true);
}

TEST_F(ToastManagerTest, ManualCloseRemovesVisibleToast) {
  const std::uint64_t id = manager_.show(Make("closable"));
  ASSERT_EQ(manager_.visible_count(), 1u);

  manager_.close(id);

  EXPECT_TRUE(manager_.empty());
}

TEST_F(ToastManagerTest, ActionCallbackRunsAndClosesToast) {
  int calls = 0;
  const std::uint64_t id =
      manager_.show(Make("act", ToastSeverity::kInfo, 5s, ToastAction{"go", [&] { ++calls; }}));

  manager_.invoke(id);

  EXPECT_EQ(calls, 1);
  EXPECT_TRUE(manager_.empty());
}

TEST_F(ToastManagerTest, ActionRunsAtMostOnce) {
  int calls = 0;
  const std::uint64_t id =
      manager_.show(Make("act", ToastSeverity::kInfo, 5s, ToastAction{"go", [&] { ++calls; }}));

  manager_.invoke(id);
  manager_.invoke(id);  // toast is already gone; must be a no-op

  EXPECT_EQ(calls, 1);
}

TEST_F(ToastManagerTest, InvokeReturnsFalseForActionlessToast) {
  const std::uint64_t id = manager_.show(Make("no-action"));

  // No action: invoke reports failure and does not close the toast.
  EXPECT_FALSE(manager_.invoke(id));
  EXPECT_EQ(manager_.visible_count(), 1u);
}

TEST_F(ToastManagerTest, InvokeReturnsTrueAndClosesWithAction) {
  int calls = 0;
  const std::uint64_t id =
      manager_.show(Make("act", ToastSeverity::kInfo, 5s, ToastAction{"go", [&] { ++calls; }}));

  EXPECT_TRUE(manager_.invoke(id));
  EXPECT_TRUE(manager_.empty());
  EXPECT_EQ(calls, 1);
}

TEST_F(ToastManagerTest, NullClockUpdateIsANoOp) {
  ToastManager manager(nullptr);  // documented: a null clock never crashes
  EXPECT_NO_FATAL_FAILURE(manager.update());
  EXPECT_NO_FATAL_FAILURE(manager.clear_all());
}

TEST_F(ToastManagerTest, FocusNavigationWraps) {
  manager_.show(Make("a"));
  manager_.show(Make("b"));
  manager_.show(Make("c"));

  EXPECT_FALSE(manager_.focus().has_value());

  manager_.move_focus(1);
  EXPECT_EQ(*manager_.focus(), 0u);
  EXPECT_EQ(manager_.visible()[0].focused, true);

  manager_.move_focus(1);
  EXPECT_EQ(*manager_.focus(), 1u);
  manager_.move_focus(1);
  EXPECT_EQ(*manager_.focus(), 2u);
  manager_.move_focus(1);  // wrap to first
  EXPECT_EQ(*manager_.focus(), 0u);
  manager_.move_focus(-1);  // wrap to last
  EXPECT_EQ(*manager_.focus(), 2u);
}

TEST_F(ToastManagerTest, FocusedIdTracksFocus) {
  const std::uint64_t first = manager_.show(Make("a"));
  manager_.show(Make("b"));

  EXPECT_FALSE(manager_.focused_id().has_value());
  manager_.set_focus(1);
  ASSERT_TRUE(manager_.focused_id().has_value());
  EXPECT_NE(manager_.focused_id().value(), first);
}

TEST_F(ToastManagerTest, TimeoutPausedWhileFocused) {
  manager_.show(Make("timed", ToastSeverity::kInfo, 5s));
  manager_.set_focus(0);

  manager_.update();  // clock baseline
  clock_->Advance(300ms);
  manager_.update();
  clock_->Advance(1s);
  manager_.update();
  clock_->Advance(5s);  // would be 6.3s elapsed: past the 5s deadline
  manager_.update();

  // The focused toast's timer is paused, so it must still be present.
  ASSERT_EQ(manager_.visible_count(), 1u);
  EXPECT_EQ(manager_.visible()[0].message, "timed");
}

TEST_F(ToastManagerTest, TimeoutResumesWhenFocusMovesAway) {
  manager_.show(Make("timed", ToastSeverity::kInfo, 5s));
  manager_.set_focus(0);

  manager_.update();
  clock_->Advance(6s);
  manager_.update();
  ASSERT_EQ(manager_.visible_count(), 1u);  // paused while focused

  manager_.set_focus(std::nullopt);  // leave focus -> timer resumes
  clock_->Advance(6s);
  manager_.update();

  EXPECT_TRUE(manager_.empty());
}

TEST_F(ToastManagerTest, ClearAllEmptiesEverythingAndDropsFocus) {
  manager_.show(Make("a"));
  manager_.show(Make("b"));
  manager_.set_focus(0);
  manager_.update();

  manager_.clear_all();

  EXPECT_TRUE(manager_.empty());
  EXPECT_FALSE(manager_.focus().has_value());
}

TEST_F(ToastManagerTest, RemovingFocusedToastDropsFocusToNullopt) {
  manager_.show(Make("a"));
  manager_.show(Make("b"));
  manager_.set_focus(0);
  ASSERT_TRUE(manager_.focus().has_value());

  manager_.close(manager_.visible()[0].id);

  EXPECT_FALSE(manager_.focus().has_value());
}

TEST_F(ToastManagerTest, FocusStaysValidWhenNonFocusedToastIsRemoved) {
  manager_.show(Make("a"));
  manager_.show(Make("b"));
  manager_.show(Make("c"));
  manager_.set_focus(2);  // on "c"

  manager_.close(manager_.visible()[0].id);  // remove "a"; focus shifts to index 1

  ASSERT_TRUE(manager_.focus().has_value());
  EXPECT_EQ(*manager_.focus(), 1u);
  ASSERT_TRUE(manager_.focused_id().has_value());
  EXPECT_EQ(manager_.focused_id().value(), manager_.visible()[1].id);
}

TEST_F(ToastManagerTest, RemoveDuringCallbackIsSafe) {
  const std::uint64_t victim = manager_.show(Make("victim"));
  int calls = 0;
  const std::uint64_t first = manager_.show(Make(
      "first", ToastSeverity::kInfo, 5s, ToastAction{"go", [&] {
                                                       ++calls;
                                                       // The callback removes a *different* toast
                                                       // and adds one while the manager's invoke()
                                                       // is mid-flight.
                                                       manager_.close(victim);
                                                       manager_.show(Make("new-from-callback"));
                                                     }}));

  manager_.invoke(first);

  EXPECT_EQ(calls, 1);
  bool victim_present = false;
  bool new_present = false;
  for (const ToastInfo& toast : manager_.visible()) {
    if (toast.message == "victim") victim_present = true;
    if (toast.message == "new-from-callback") new_present = true;
  }
  EXPECT_FALSE(victim_present);
  EXPECT_TRUE(new_present);
}

TEST_F(ToastManagerTest, TimedExpiryPromotesFromQueueAndKeepsFocusValid) {
  auto fake = std::make_shared<FakeClock>();
  ToastManager small(fake, /*max_visible=*/1);

  small.show(Make("one", ToastSeverity::kInfo, 5s));    // visible
  small.show(Make("two", ToastSeverity::kInfo, 5s));    // queued
  small.show(Make("three", ToastSeverity::kInfo, 5s));  // queued
  ASSERT_EQ(small.visible_count(), 1u);
  ASSERT_EQ(small.queued_count(), 2u);

  small.update();     // clock baseline
  fake->Advance(6s);  // "one" expires
  small.update();

  // The oldest queued toast is promoted into the freed slot.
  ASSERT_EQ(small.visible_count(), 1u);
  ASSERT_EQ(small.queued_count(), 1u);
  EXPECT_EQ(small.visible()[0].message, "two");

  fake->Advance(6s);
  small.update();
  ASSERT_EQ(small.visible()[0].message, "three");
  EXPECT_TRUE(small.empty() == false);

  fake->Advance(6s);
  small.update();
  EXPECT_TRUE(small.empty());

  // focus stays valid (unfocused) through removals/promotions.
  EXPECT_FALSE(small.focus().has_value());
}

TEST_F(ToastManagerTest, ZeroMaxVisibleIsClampedToOne) {
  auto fake = std::make_shared<FakeClock>();
  ToastManager clamped(fake, /*max_visible=*/0);

  clamped.show(Make("a"));
  clamped.show(Make("b"));

  EXPECT_EQ(clamped.max_visible(), 1u);
  EXPECT_EQ(clamped.visible_count(), 1u);
  EXPECT_EQ(clamped.queued_count(), 1u);
}

}  // namespace
}  // namespace terminal_ui_kit