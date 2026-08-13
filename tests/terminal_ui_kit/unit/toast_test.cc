#include "terminal_ui_kit/components/toast.h"

#include <chrono>
#include <cstddef>
#include <random>
#include <string>

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

using TimePoint = std::chrono::steady_clock::time_point;
using Duration = std::chrono::steady_clock::duration;

// Deterministic fake clock: time only advances when the test tells it to, so
// expiry/pause/resume behaviour is reproducible and needs no real sleeps.
class FakeClock {
 public:
  ToastClock MakeClock() {
    return [this] { return now_; };
  }
  void Advance(std::chrono::milliseconds ms) { now_ += ms; }
  TimePoint Now() const { return now_; }

 private:
  TimePoint now_{};
};

ToastManager MakeManager(FakeClock& clock, std::size_t max_visible = 5) {
  ToastManagerOptions options;
  options.max_visible = max_visible;
  options.clock = clock.MakeClock();
  return ToastManager(options);
}

ToastOptions Timed(std::string message, std::chrono::milliseconds duration,
                   ToastSeverity severity = ToastSeverity::kInfo) {
  ToastOptions options;
  options.message = std::move(message);
  options.severity = severity;
  options.duration = duration;
  return options;
}

TEST(ToastManager, OneToast) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  std::size_t id = manager.Show(Timed("hello", std::chrono::seconds(5)));

  EXPECT_EQ(manager.visible_count(), 1u);
  ASSERT_FALSE(manager.Visible().empty());
  EXPECT_EQ(manager.Visible()[0].id, id);
  EXPECT_EQ(manager.Visible()[0].message, "hello");
  EXPECT_EQ(manager.Visible()[0].severity, ToastSeverity::kInfo);
  EXPECT_TRUE(manager.Visible()[0].remaining.has_value());
}

TEST(ToastManager, EachSeverity) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  manager.Show(Timed("info", std::chrono::seconds(5), ToastSeverity::kInfo));
  manager.Show(Timed("ok", std::chrono::seconds(5), ToastSeverity::kSuccess));
  manager.Show(Timed("warn", std::chrono::seconds(5), ToastSeverity::kWarning));
  manager.Show(Timed("err", std::chrono::seconds(5), ToastSeverity::kError));

  EXPECT_EQ(manager.visible_count(), 4u);
  EXPECT_EQ(manager.Visible()[0].severity, ToastSeverity::kInfo);
  EXPECT_EQ(manager.Visible()[1].severity, ToastSeverity::kSuccess);
  EXPECT_EQ(manager.Visible()[2].severity, ToastSeverity::kWarning);
  EXPECT_EQ(manager.Visible()[3].severity, ToastSeverity::kError);
}

TEST(ToastManager, QueueOrderingIsFifo) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock, /*max_visible=*/2);

  std::size_t first = manager.Show(Timed("a", std::chrono::seconds(5)));
  std::size_t second = manager.Show(Timed("b", std::chrono::seconds(5)));
  std::size_t third = manager.Show(Timed("c", std::chrono::seconds(5)));

  EXPECT_EQ(manager.visible_count(), 2u);
  EXPECT_EQ(manager.queued_count(), 1u);
  EXPECT_EQ(manager.Visible()[0].id, first);
  EXPECT_EQ(manager.Visible()[1].id, second);
  EXPECT_EQ(manager.Visible()[2].id, third);  // the queued one trails in order

  // Closing the oldest visible toast promotes the first queued one.
  manager.Close(first);
  EXPECT_EQ(manager.visible_count(), 2u);
  EXPECT_EQ(manager.Visible()[0].id, second);
  EXPECT_EQ(manager.Visible()[1].id, third);
  EXPECT_EQ(manager.queued_count(), 0u);
}

TEST(ToastManager, VisibleCountLimit) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock, /*max_visible=*/3);

  manager.Show(Timed("1", std::chrono::seconds(5)));
  manager.Show(Timed("2", std::chrono::seconds(5)));
  manager.Show(Timed("3", std::chrono::seconds(5)));
  manager.Show(Timed("4", std::chrono::seconds(5)));
  manager.Show(Timed("5", std::chrono::seconds(5)));

  EXPECT_EQ(manager.max_visible(), 3u);
  EXPECT_EQ(manager.visible_count(), 3u);
  EXPECT_EQ(manager.queued_count(), 2u);
}

TEST(ToastManager, TimedExpiry) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  manager.Show(Timed("soon", std::chrono::seconds(1)));
  manager.Show(Timed("later", std::chrono::seconds(10)));
  manager.Tick();
  clock.Advance(std::chrono::seconds(2));
  manager.Tick();

  EXPECT_EQ(manager.visible_count(), 1u);
  EXPECT_EQ(manager.Visible()[0].message, "later");
  EXPECT_EQ(manager.queued_count(), 0u);
}

TEST(ToastManager, PersistentToastNeverExpires) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  ToastOptions options;
  options.message = "sticky";
  options.duration = std::nullopt;  // persistent
  manager.Show(options);
  manager.Tick();

  clock.Advance(std::chrono::hours(24));
  manager.Tick();
  manager.Tick();

  EXPECT_EQ(manager.visible_count(), 1u);
  EXPECT_EQ(manager.Visible()[0].message, "sticky");
}

TEST(ToastManager, ManualClose) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  std::size_t id = manager.Show(Timed("x", std::chrono::seconds(5)));
  manager.Close(id);
  manager.Close(id);  // closing twice is a no-op
  EXPECT_TRUE(manager.empty());
  EXPECT_EQ(manager.visible_count(), 0u);
}

TEST(ToastManager, ActionCallbackInvoked) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  int calls = 0;
  ToastOptions options = Timed("act", std::chrono::seconds(5));
  options.action = ToastAction{"undo", [&calls] { ++calls; }};
  std::size_t id = manager.Show(options);

  manager.SetFocused(id);
  EXPECT_TRUE(manager.InvokeFocusedAction());
  EXPECT_EQ(calls, 1);
  // Invoking removed the toast.
  EXPECT_TRUE(manager.empty());
}

TEST(ToastManager, ActionCallbackAtMostOnce) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  int calls = 0;
  ToastOptions options = Timed("act", std::chrono::seconds(5));
  options.action = ToastAction{"undo", [&calls] { ++calls; }};
  std::size_t id = manager.Show(options);

  manager.SetFocused(id);
  EXPECT_TRUE(manager.InvokeFocusedAction());
  // Toast is gone, so no second invocation can happen.
  EXPECT_FALSE(manager.InvokeFocusedAction());
  EXPECT_EQ(calls, 1);
}

TEST(ToastManager, FocusNavigation) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock, /*max_visible=*/3);

  std::size_t a = manager.Show(Timed("a", std::chrono::seconds(5)));
  std::size_t b = manager.Show(Timed("b", std::chrono::seconds(5)));

  // Tab from no selection selects the first visible toast.
  EXPECT_TRUE(manager.MoveFocus(1));
  EXPECT_TRUE(manager.HasFocus());
  EXPECT_EQ(manager.FocusedId(), a);

  EXPECT_TRUE(manager.MoveFocus(1));
  EXPECT_EQ(manager.FocusedId(), b);

  // Stepping past the last visible toast clears the selection (timeouts resume).
  EXPECT_TRUE(manager.MoveFocus(1));
  EXPECT_FALSE(manager.HasFocus());

  // Shift+Tab from no selection selects the last visible toast.
  EXPECT_TRUE(manager.MoveFocus(-1));
  EXPECT_EQ(manager.FocusedId(), b);
  EXPECT_TRUE(manager.MoveFocus(-1));
  EXPECT_EQ(manager.FocusedId(), a);
  EXPECT_TRUE(manager.MoveFocus(-1));
  EXPECT_FALSE(manager.HasFocus());
}

TEST(ToastManager, TimeoutPausesWhileFocused) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  std::size_t id = manager.Show(Timed("pause", std::chrono::seconds(5)));
  manager.Tick();
  clock.Advance(std::chrono::seconds(3));
  manager.Tick();  // 3s consumed, 2s remain
  EXPECT_EQ(manager.visible_count(), 1u);

  manager.SetFocused(id);
  // A huge amount of time passes while focused: remaining time is frozen.
  clock.Advance(std::chrono::hours(1));
  manager.Tick();
  manager.Tick();
  EXPECT_EQ(manager.visible_count(), 1u);
  EXPECT_EQ(manager.Visible()[0].id, id);
}

TEST(ToastManager, TimeoutResumesAfterUnfocus) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  std::size_t id = manager.Show(Timed("resume", std::chrono::seconds(5)));
  manager.Tick();
  clock.Advance(std::chrono::seconds(3));  // 2s remain
  manager.Tick();
  manager.SetFocused(id);
  clock.Advance(std::chrono::hours(1));  // frozen while focused
  manager.Tick();

  manager.ClearFocus();
  clock.Advance(std::chrono::seconds(3));  // 3s passes after resume > 2s left
  manager.Tick();
  EXPECT_TRUE(manager.empty());
  EXPECT_EQ(manager.visible_count(), 0u);
}

TEST(ToastManager, ClearAll) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  manager.Show(Timed("a", std::chrono::seconds(5)));
  manager.Show(Timed("b", std::chrono::seconds(5)));
  manager.SetFocused(0);

  manager.ClearAll();
  EXPECT_TRUE(manager.empty());
  EXPECT_FALSE(manager.HasFocus());
}

TEST(ToastManager, RemovalDuringCallbackIsSafe) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock, /*max_visible=*/3);

  std::size_t other = manager.Show(Timed("other", std::chrono::seconds(5)));

  ToastOptions options = Timed("act", std::chrono::seconds(5));
  options.action = ToastAction{"clear",
                               // The callback removes another toast and also clears everything.
                               [&manager, other] {
                                 manager.Close(other);
                                 manager.ClearAll();
                               }};
  std::size_t action_id = manager.Show(options);
  manager.Show(Timed("queued", std::chrono::seconds(5)));

  manager.SetFocused(action_id);
  EXPECT_TRUE(manager.InvokeFocusedAction());
  // Must not crash; state must remain consistent.
  EXPECT_TRUE(manager.empty());
  EXPECT_FALSE(manager.HasFocus());
}

TEST(ToastManager, LongMessagePreserved) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  std::string long_message(10000, 'x');
  manager.Show(Timed(long_message, std::chrono::seconds(5)));

  EXPECT_EQ(manager.Visible()[0].message.size(), long_message.size());
  EXPECT_EQ(manager.Visible()[0].message, long_message);
}

TEST(ToastManager, FocusRemainsValidAfterRemovalAndExpiry) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock, /*max_visible=*/3);

  // Expiry while nothing is focused removes every toast and clears focus
  // rather than leaving it dangling.
  manager.Show(Timed("a", std::chrono::seconds(1)));
  manager.Show(Timed("b", std::chrono::seconds(1)));
  manager.Show(Timed("c", std::chrono::seconds(1)));
  manager.Tick();  // initializing tick (establishes last_tick_, no expiry)
  clock.Advance(std::chrono::seconds(5));
  manager.Tick();
  EXPECT_TRUE(manager.empty());
  EXPECT_FALSE(manager.HasFocus());

  // Manual removal of the focused toast clamps focus onto a survivor.
  std::size_t x = manager.Show(Timed("x", std::chrono::seconds(5)));
  std::size_t y = manager.Show(Timed("y", std::chrono::seconds(5)));
  manager.SetFocused(x);
  manager.Close(x);
  EXPECT_EQ(manager.visible_count(), 1u);
  EXPECT_TRUE(manager.HasFocus());
  EXPECT_EQ(manager.FocusedId(), y);

  manager.Close(y);
  EXPECT_TRUE(manager.empty());
  EXPECT_FALSE(manager.HasFocus());
}

TEST(ToastManager, EmptyMessagePolicy) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  // Empty messages are permitted and produce a valid (icon/action-only) toast.
  std::size_t id = manager.Show(Timed("", std::chrono::seconds(5)));
  EXPECT_EQ(manager.visible_count(), 1u);
  EXPECT_EQ(manager.Visible()[0].id, id);
  EXPECT_TRUE(manager.Visible()[0].message.empty());
}

TEST(ToastManager, DeterministicFakeClock) {
  // The same clock sequence always yields the same outcome.
  FakeClock clock_a;
  ToastManager manager_a = MakeManager(clock_a);
  FakeClock clock_b;
  ToastManager manager_b = MakeManager(clock_b);

  auto drive = [](ToastManager& manager, FakeClock& clock) {
    manager.Show(Timed("x", std::chrono::seconds(5)));
    manager.Tick();
    clock.Advance(std::chrono::seconds(1));
    manager.Tick();
    clock.Advance(std::chrono::seconds(1));
    manager.Tick();
    clock.Advance(std::chrono::seconds(2));
    manager.Tick();
    return manager.visible_count();
  };

  EXPECT_EQ(drive(manager_a, clock_a), drive(manager_b, clock_b));
  EXPECT_EQ(manager_a.visible_count(), 1u);
  EXPECT_FALSE(manager_a.empty());
}

TEST(ToastManager, RandomizedStressKeepsInvariants) {
  // A deterministic pseudo-random mix of operations. Runs under ASan/UBSan to
  // prove no use-after-free, overflow or dangling focus on arbitrary input.
  FakeClock clock;
  ToastManager manager = MakeManager(clock, /*max_visible=*/5);
  std::mt19937 rng(20240813u);
  std::vector<std::size_t> ids;
  for (int step = 0; step < 4000; ++step) {
    const std::size_t op = static_cast<std::size_t>(rng()) % 7;
    switch (op) {
      case 0: {  // show a random toast
        ToastOptions options;
        options.message = "m" + std::to_string(step);
        options.severity = static_cast<ToastSeverity>(static_cast<std::size_t>(rng()) % 4);
        if ((rng() % 5) != 0) {
          options.duration = std::chrono::milliseconds(static_cast<int>(rng()) % 500);
        }
        ids.push_back(manager.Show(options));
        break;
      }
      case 1:  // close a random id
        if (!ids.empty()) {
          manager.Close(ids[static_cast<std::size_t>(rng()) % ids.size()]);
        }
        break;
      case 2:  // clear all
        manager.ClearAll();
        ids.clear();
        break;
      case 3:  // move focus by a random delta
        manager.MoveFocus(static_cast<int>(rng() % 3) - 1);
        break;
      case 4:  // invoke focused action
        manager.InvokeFocusedAction();
        break;
      case 5:  // close focused
        manager.CloseFocused();
        break;
      case 6:  // advance time and tick
        clock.Advance(std::chrono::milliseconds(static_cast<int>(rng()) % 100));
        manager.Tick();
        break;
      default:
        break;
    }

    // Invariants: never exceed the bound, focus always points at a visible
    // toast, and time never runs *backwards*.
    ASSERT_LE(manager.visible_count(), manager.max_visible());
    if (manager.HasFocus()) {
      const std::optional<std::size_t> focused = manager.FocusedId();
      const auto& visible = manager.Visible();
      bool found = false;
      for (std::size_t i = 0; i < manager.visible_count() && i < visible.size(); ++i) {
        if (visible[i].id == *focused) {
          found = true;
          break;
        }
      }
      ASSERT_TRUE(found);
    }
  }
}

}  // namespace
}  // namespace terminal_ui_kit
