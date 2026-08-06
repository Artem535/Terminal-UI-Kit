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
  for (const ToastInfo& toast : manager.Visible()) {
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
  manager_.Show(Make("hello"));

  ASSERT_EQ(manager_.VisibleCount(), 1u);
  ASSERT_EQ(manager_.Visible().size(), 1u);
  EXPECT_EQ(manager_.Visible()[0].message, "hello");
  EXPECT_EQ(manager_.QueuedCount(), 0u);
}

TEST_F(ToastManagerTest, EachSeverityIsPreserved) {
  manager_.Show(Make("i", ToastSeverity::kInfo));
  manager_.Show(Make("s", ToastSeverity::kSuccess));
  manager_.Show(Make("w", ToastSeverity::kWarning));
  manager_.Show(Make("e", ToastSeverity::kError));

  const std::vector<ToastInfo> visible = manager_.Visible();
  ASSERT_EQ(visible.size(), 3u);
  EXPECT_EQ(visible[0].severity, ToastSeverity::kInfo);
  EXPECT_EQ(visible[1].severity, ToastSeverity::kSuccess);
  EXPECT_EQ(visible[2].severity, ToastSeverity::kWarning);
}

TEST_F(ToastManagerTest, ShowReturnsStableIds) {
  const std::uint64_t first = manager_.Show(Make("a"));
  const std::uint64_t second = manager_.Show(Make("b"));

  EXPECT_NE(first, second);
  EXPECT_EQ(manager_.Visible()[0].id, first);
  EXPECT_EQ(manager_.Visible()[1].id, second);
}

TEST_F(ToastManagerTest, QueuePreservesInsertionOrder) {
  // max_visible = 3 (from fixture). Show 6 toasts.
  for (int i = 1; i <= 6; ++i) {
    manager_.Show(Make("toast-" + std::to_string(i)));
  }

  ASSERT_EQ(manager_.VisibleCount(), 3u);
  ASSERT_EQ(manager_.QueuedCount(), 3u);
  EXPECT_EQ(Messages(manager_), (std::vector<std::string>{"toast-1", "toast-2", "toast-3"}));

  // Close the oldest visible; the longest-waiting queued toast is promoted.
  manager_.Close(manager_.Visible()[0].id);
  ASSERT_EQ(manager_.VisibleCount(), 3u);
  EXPECT_EQ(Messages(manager_), (std::vector<std::string>{"toast-2", "toast-3", "toast-4"}));
}

TEST_F(ToastManagerTest, VisibleCountRespectsLimit) {
  // max_visible = 3; adding 5 leaves 2 queued.
  for (int i = 1; i <= 5; ++i) {
    const std::uint64_t id = manager_.Show(Make("t" + std::to_string(i)));
    EXPECT_NE(id, 0u);
  }

  EXPECT_EQ(manager_.VisibleCount(), 3u);
  EXPECT_EQ(manager_.QueuedCount(), 2u);
  EXPECT_EQ(manager_.MaxVisible(), 3u);
}

TEST_F(ToastManagerTest, TimedToastExpires) {
  manager_.Show(Make("ephemeral", ToastSeverity::kInfo, 5s));

  manager_.Update();  // establish the clock baseline
  ASSERT_EQ(manager_.VisibleCount(), 1u);
  clock_->Advance(6s);
  manager_.Update();

  EXPECT_TRUE(manager_.Empty());
}

TEST_F(ToastManagerTest, TimedToastNeverExpiresBeforeItsDeadline) {
  manager_.Show(Make("ephemeral", ToastSeverity::kInfo, 5s));

  manager_.Update();
  clock_->Advance(4s);
  manager_.Update();

  ASSERT_EQ(manager_.VisibleCount(), 1u);
  EXPECT_EQ(manager_.Visible()[0].message, "ephemeral");
}

TEST_F(ToastManagerTest, PersistentToastNeverTimesOut) {
  manager_.Show(Persistent("sticky"));

  manager_.Update();
  clock_->Advance(3600s);
  manager_.Update();

  ASSERT_EQ(manager_.VisibleCount(), 1u);
  EXPECT_EQ(manager_.Visible()[0].persistent, true);
}

TEST_F(ToastManagerTest, ManualCloseRemovesVisibleToast) {
  const std::uint64_t id = manager_.Show(Make("closable"));
  ASSERT_EQ(manager_.VisibleCount(), 1u);

  manager_.Close(id);

  EXPECT_TRUE(manager_.Empty());
}

TEST_F(ToastManagerTest, ActionCallbackRunsAndClosesToast) {
  int calls = 0;
  const std::uint64_t id =
      manager_.Show(Make("act", ToastSeverity::kInfo, 5s, ToastAction{"go", [&] { ++calls; }}));

  manager_.Invoke(id);

  EXPECT_EQ(calls, 1);
  EXPECT_TRUE(manager_.Empty());
}

TEST_F(ToastManagerTest, ActionRunsAtMostOnce) {
  int calls = 0;
  const std::uint64_t id =
      manager_.Show(Make("act", ToastSeverity::kInfo, 5s, ToastAction{"go", [&] { ++calls; }}));

  manager_.Invoke(id);
  manager_.Invoke(id);  // toast is already gone; must be a no-op
  manager_.Invoke(id);

  EXPECT_EQ(calls, 1);
}

TEST_F(ToastManagerTest, FocusNavigationWraps) {
  manager_.Show(Make("a"));
  manager_.Show(Make("b"));
  manager_.Show(Make("c"));

  EXPECT_FALSE(manager_.Focus().has_value());

  manager_.MoveFocus(1);
  EXPECT_EQ(*manager_.Focus(), 0u);
  EXPECT_EQ(manager_.Visible()[0].focused, true);

  manager_.MoveFocus(1);
  EXPECT_EQ(*manager_.Focus(), 1u);
  manager_.MoveFocus(1);
  EXPECT_EQ(*manager_.Focus(), 2u);
  manager_.MoveFocus(1);  // wrap to first
  EXPECT_EQ(*manager_.Focus(), 0u);
  manager_.MoveFocus(-1);  // wrap to last
  EXPECT_EQ(*manager_.Focus(), 2u);
}

TEST_F(ToastManagerTest, FocusedIdTracksFocus) {
  const std::uint64_t first = manager_.Show(Make("a"));
  manager_.Show(Make("b"));

  EXPECT_FALSE(manager_.FocusedId().has_value());
  manager_.SetFocus(1);
  ASSERT_TRUE(manager_.FocusedId().has_value());
  EXPECT_NE(manager_.FocusedId().value(), first);
}

TEST_F(ToastManagerTest, TimeoutPausedWhileFocused) {
  manager_.Show(Make("timed", ToastSeverity::kInfo, 5s));
  manager_.SetFocus(0);

  manager_.Update();  // clock baseline
  clock_->Advance(300ms);
  manager_.Update();
  clock_->Advance(1s);
  manager_.Update();
  clock_->Advance(5s);  // would be 6.3s elapsed: past the 5s deadline
  manager_.Update();

  // The focused toast's timer is paused, so it must still be present.
  ASSERT_EQ(manager_.VisibleCount(), 1u);
  EXPECT_EQ(manager_.Visible()[0].message, "timed");
}

TEST_F(ToastManagerTest, TimeoutResumesWhenFocusMovesAway) {
  manager_.Show(Make("timed", ToastSeverity::kInfo, 5s));
  manager_.SetFocus(0);

  manager_.Update();
  clock_->Advance(6s);
  manager_.Update();
  ASSERT_EQ(manager_.VisibleCount(), 1u);  // paused while focused

  manager_.SetFocus(std::nullopt);  // leave focus -> timer resumes
  clock_->Advance(6s);
  manager_.Update();

  EXPECT_TRUE(manager_.Empty());
}

TEST_F(ToastManagerTest, ClearAllEmptiesEverythingAndDropsFocus) {
  manager_.Show(Make("a"));
  manager_.Show(Make("b"));
  manager_.SetFocus(0);
  manager_.Update();

  manager_.ClearAll();

  EXPECT_TRUE(manager_.Empty());
  EXPECT_FALSE(manager_.Focus().has_value());
}

TEST_F(ToastManagerTest, RemovingFocusedToastDropsFocusToNullopt) {
  manager_.Show(Make("a"));
  manager_.Show(Make("b"));
  manager_.SetFocus(0);
  ASSERT_TRUE(manager_.Focus().has_value());

  manager_.Close(manager_.Visible()[0].id);

  EXPECT_FALSE(manager_.Focus().has_value());
}

TEST_F(ToastManagerTest, FocusStaysValidWhenNonFocusedToastIsRemoved) {
  manager_.Show(Make("a"));
  manager_.Show(Make("b"));
  manager_.Show(Make("c"));
  manager_.SetFocus(2);  // on "c"

  manager_.Close(manager_.Visible()[0].id);  // remove "a"; focus shifts to index 1

  ASSERT_TRUE(manager_.Focus().has_value());
  EXPECT_EQ(*manager_.Focus(), 1u);
  ASSERT_TRUE(manager_.FocusedId().has_value());
  EXPECT_EQ(manager_.FocusedId().value(), manager_.Visible()[1].id);
}

TEST_F(ToastManagerTest, RemoveDuringCallbackIsSafe) {
  const std::uint64_t victim = manager_.Show(Make("victim"));
  int calls = 0;
  const std::uint64_t first = manager_.Show(Make(
      "first", ToastSeverity::kInfo, 5s, ToastAction{"go", [&] {
                                                       ++calls;
                                                       // The callback removes a *different* toast
                                                       // and adds one while the manager's Invoke()
                                                       // is mid-flight.
                                                       manager_.Close(victim);
                                                       manager_.Show(Make("new-from-callback"));
                                                     }}));

  manager_.Invoke(first);

  EXPECT_EQ(calls, 1);
  bool victim_present = false;
  bool new_present = false;
  for (const ToastInfo& toast : manager_.Visible()) {
    if (toast.message == "victim") victim_present = true;
    if (toast.message == "new-from-callback") new_present = true;
  }
  EXPECT_FALSE(victim_present);
  EXPECT_TRUE(new_present);
}

TEST_F(ToastManagerTest, TimedExpiryPromotesFromQueueAndKeepsFocusValid) {
  auto fake = std::make_shared<FakeClock>();
  ToastManager small(fake, /*max_visible=*/1);

  small.Show(Make("one", ToastSeverity::kInfo, 5s));    // visible
  small.Show(Make("two", ToastSeverity::kInfo, 5s));    // queued
  small.Show(Make("three", ToastSeverity::kInfo, 5s));  // queued
  ASSERT_EQ(small.VisibleCount(), 1u);
  ASSERT_EQ(small.QueuedCount(), 2u);

  small.Update();     // clock baseline
  fake->Advance(6s);  // "one" expires
  small.Update();

  // The oldest queued toast is promoted into the freed slot.
  ASSERT_EQ(small.VisibleCount(), 1u);
  ASSERT_EQ(small.QueuedCount(), 1u);
  EXPECT_EQ(small.Visible()[0].message, "two");

  fake->Advance(6s);
  small.Update();
  ASSERT_EQ(small.Visible()[0].message, "three");
  EXPECT_TRUE(small.Empty() == false);

  fake->Advance(6s);
  small.Update();
  EXPECT_TRUE(small.Empty());

  // Focus stays valid (unfocused) through removals/promotions.
  EXPECT_FALSE(small.Focus().has_value());
}

TEST_F(ToastManagerTest, ZeroMaxVisibleIsClampedToOne) {
  auto fake = std::make_shared<FakeClock>();
  ToastManager clamped(fake, /*max_visible=*/0);

  clamped.Show(Make("a"));
  clamped.Show(Make("b"));

  EXPECT_EQ(clamped.MaxVisible(), 1u);
  EXPECT_EQ(clamped.VisibleCount(), 1u);
  EXPECT_EQ(clamped.QueuedCount(), 1u);
}

}  // namespace
}  // namespace terminal_ui_kit