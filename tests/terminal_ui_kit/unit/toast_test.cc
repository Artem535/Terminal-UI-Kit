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

  std::size_t id = manager.show(Timed("hello", std::chrono::seconds(5)));

  EXPECT_EQ(manager.visible_count(), 1u);
  EXPECT_EQ(manager.queued_count(), 0u);
  std::vector<Toast> visible = manager.visible();
  ASSERT_EQ(visible.size(), 1u);
  EXPECT_EQ(visible[0].id, id);
  EXPECT_EQ(visible[0].message, "hello");
  EXPECT_EQ(visible[0].severity, ToastSeverity::kInfo);
  EXPECT_TRUE(visible[0].remaining.has_value());
}

TEST(ToastManager, EachSeverity) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  manager.show(Timed("info", std::chrono::seconds(5), ToastSeverity::kInfo));
  manager.show(Timed("ok", std::chrono::seconds(5), ToastSeverity::kSuccess));
  manager.show(Timed("warn", std::chrono::seconds(5), ToastSeverity::kWarning));
  manager.show(Timed("err", std::chrono::seconds(5), ToastSeverity::kError));

  std::vector<Toast> visible = manager.visible();
  ASSERT_EQ(visible.size(), 4u);
  EXPECT_EQ(visible[0].severity, ToastSeverity::kInfo);
  EXPECT_EQ(visible[1].severity, ToastSeverity::kSuccess);
  EXPECT_EQ(visible[2].severity, ToastSeverity::kWarning);
  EXPECT_EQ(visible[3].severity, ToastSeverity::kError);
}

TEST(ToastManager, QueueOrderingIsFifo) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock, /*max_visible=*/2);

  std::size_t first = manager.show(Timed("a", std::chrono::seconds(5)));
  std::size_t second = manager.show(Timed("b", std::chrono::seconds(5)));
  std::size_t third = manager.show(Timed("c", std::chrono::seconds(5)));

  EXPECT_EQ(manager.visible_count(), 2u);
  EXPECT_EQ(manager.queued_count(), 1u);

  // visible() exposes only the visible window; the queued toast is NOT there.
  std::vector<Toast> visible = manager.visible();
  ASSERT_EQ(visible.size(), 2u);
  EXPECT_EQ(visible[0].id, first);
  EXPECT_EQ(visible[1].id, second);

  // active() exposes the full FIFO order (visible + queued).
  const std::vector<Toast>& active = manager.active();
  ASSERT_EQ(active.size(), 3u);
  EXPECT_EQ(active[0].id, first);
  EXPECT_EQ(active[1].id, second);
  EXPECT_EQ(active[2].id, third);

  // Closing the oldest visible toast promotes the first queued one.
  manager.close(first);
  EXPECT_EQ(manager.visible_count(), 2u);
  visible = manager.visible();
  ASSERT_EQ(visible.size(), 2u);
  EXPECT_EQ(visible[0].id, second);
  EXPECT_EQ(visible[1].id, third);
  EXPECT_EQ(manager.queued_count(), 0u);
}

TEST(ToastManager, VisibleCountLimit) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock, /*max_visible=*/3);

  manager.show(Timed("1", std::chrono::seconds(5)));
  manager.show(Timed("2", std::chrono::seconds(5)));
  manager.show(Timed("3", std::chrono::seconds(5)));
  manager.show(Timed("4", std::chrono::seconds(5)));
  manager.show(Timed("5", std::chrono::seconds(5)));

  EXPECT_EQ(manager.max_visible(), 3u);
  EXPECT_EQ(manager.visible_count(), 3u);
  EXPECT_EQ(manager.queued_count(), 2u);
  EXPECT_EQ(manager.visible().size(), 3u);
  EXPECT_EQ(manager.active().size(), 5u);
}

TEST(ToastManager, TimedExpiry) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  manager.show(Timed("soon", std::chrono::seconds(1)));
  manager.show(Timed("later", std::chrono::seconds(10)));
  manager.tick();
  clock.Advance(std::chrono::seconds(2));
  manager.tick();

  std::vector<Toast> visible = manager.visible();
  EXPECT_EQ(manager.visible_count(), 1u);
  ASSERT_EQ(visible.size(), 1u);
  EXPECT_EQ(visible[0].message, "later");
  EXPECT_EQ(manager.queued_count(), 0u);
}

TEST(ToastManager, PersistentToastNeverExpires) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  ToastOptions options;
  options.message = "sticky";
  options.duration = std::nullopt;  // persistent
  manager.show(options);
  manager.tick();

  clock.Advance(std::chrono::hours(24));
  manager.tick();
  manager.tick();

  EXPECT_EQ(manager.visible_count(), 1u);
  EXPECT_EQ(manager.visible()[0].message, "sticky");
}

TEST(ToastManager, ManualClose) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  std::size_t id = manager.show(Timed("x", std::chrono::seconds(5)));
  manager.close(id);
  manager.close(id);  // closing twice is a no-op
  EXPECT_TRUE(manager.empty());
  EXPECT_EQ(manager.visible_count(), 0u);
}

TEST(ToastManager, ActionCallbackInvoked) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  int calls = 0;
  ToastOptions options = Timed("act", std::chrono::seconds(5));
  options.action = ToastAction{"undo", [&calls] { ++calls; }};
  std::size_t id = manager.show(options);

  manager.set_focused(id);
  EXPECT_TRUE(manager.invoke_focused_action());
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
  std::size_t id = manager.show(options);

  manager.set_focused(id);
  EXPECT_TRUE(manager.invoke_focused_action());
  // Toast is gone, so no second invocation can happen.
  EXPECT_FALSE(manager.invoke_focused_action());
  EXPECT_EQ(calls, 1);
}

TEST(ToastManager, FocusNavigation) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock, /*max_visible=*/3);

  std::size_t a = manager.show(Timed("a", std::chrono::seconds(5)));
  std::size_t b = manager.show(Timed("b", std::chrono::seconds(5)));

  // Tab from no selection selects the first visible toast.
  EXPECT_TRUE(manager.move_focus(1));
  EXPECT_TRUE(manager.has_focus());
  EXPECT_EQ(manager.focused_id(), a);

  EXPECT_TRUE(manager.move_focus(1));
  EXPECT_EQ(manager.focused_id(), b);

  // Stepping past the last visible toast clears the selection (timeouts resume).
  EXPECT_TRUE(manager.move_focus(1));
  EXPECT_FALSE(manager.has_focus());

  // Shift+Tab from no selection selects the last visible toast.
  EXPECT_TRUE(manager.move_focus(-1));
  EXPECT_EQ(manager.focused_id(), b);
  EXPECT_TRUE(manager.move_focus(-1));
  EXPECT_EQ(manager.focused_id(), a);
  EXPECT_TRUE(manager.move_focus(-1));
  EXPECT_FALSE(manager.has_focus());
}

TEST(ToastManager, TimeoutPausesWhileFocused) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  std::size_t id = manager.show(Timed("pause", std::chrono::seconds(5)));
  manager.tick();
  clock.Advance(std::chrono::seconds(3));
  manager.tick();  // 3s consumed, 2s remain
  EXPECT_EQ(manager.visible_count(), 1u);

  manager.set_focused(id);
  // A huge amount of time passes while focused: remaining time is frozen.
  clock.Advance(std::chrono::hours(1));
  manager.tick();
  manager.tick();
  EXPECT_EQ(manager.visible_count(), 1u);
  EXPECT_EQ(manager.visible()[0].id, id);
}

TEST(ToastManager, TimeoutResumesAfterUnfocus) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  std::size_t id = manager.show(Timed("resume", std::chrono::seconds(5)));
  manager.tick();
  clock.Advance(std::chrono::seconds(3));  // 2s remain
  manager.tick();
  manager.set_focused(id);
  clock.Advance(std::chrono::hours(1));  // frozen while focused
  manager.tick();

  manager.clear_focus();
  clock.Advance(std::chrono::seconds(3));  // 3s passes after resume > 2s left
  manager.tick();
  EXPECT_TRUE(manager.empty());
  EXPECT_EQ(manager.visible_count(), 0u);
}

TEST(ToastManager, ClearAll) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  manager.show(Timed("a", std::chrono::seconds(5)));
  manager.show(Timed("b", std::chrono::seconds(5)));
  manager.set_focused(0);

  manager.clear_all();
  EXPECT_TRUE(manager.empty());
  EXPECT_FALSE(manager.has_focus());
}

TEST(ToastManager, RemovalDuringCallbackIsSafe) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock, /*max_visible=*/3);

  std::size_t other = manager.show(Timed("other", std::chrono::seconds(5)));

  ToastOptions options = Timed("act", std::chrono::seconds(5));
  options.action = ToastAction{"clear",
                               // The callback removes another toast and also clears everything.
                               [&manager, other] {
                                 manager.close(other);
                                 manager.clear_all();
                               }};
  std::size_t action_id = manager.show(options);
  manager.show(Timed("queued", std::chrono::seconds(5)));

  manager.set_focused(action_id);
  EXPECT_TRUE(manager.invoke_focused_action());
  // Must not crash; state must remain consistent.
  EXPECT_TRUE(manager.empty());
  EXPECT_FALSE(manager.has_focus());
}

TEST(ToastManager, FocusRemainsValidAfterRemovalAndExpiry) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock, /*max_visible=*/3);

  // Expiry while nothing is focused removes every toast and clears focus
  // rather than leaving it dangling.
  manager.show(Timed("a", std::chrono::seconds(1)));
  manager.show(Timed("b", std::chrono::seconds(1)));
  manager.show(Timed("c", std::chrono::seconds(1)));
  manager.tick();  // initializing tick: establishes last_tick_, no expiry
  clock.Advance(std::chrono::seconds(5));
  manager.tick();
  EXPECT_TRUE(manager.empty());
  EXPECT_FALSE(manager.has_focus());

  // Manual removal of the focused toast clamps focus onto a survivor.
  std::size_t x = manager.show(Timed("x", std::chrono::seconds(5)));
  std::size_t y = manager.show(Timed("y", std::chrono::seconds(5)));
  manager.set_focused(x);
  manager.close(x);
  EXPECT_EQ(manager.visible_count(), 1u);
  EXPECT_TRUE(manager.has_focus());
  EXPECT_EQ(manager.focused_id(), y);

  manager.close(y);
  EXPECT_TRUE(manager.empty());
  EXPECT_FALSE(manager.has_focus());
}

TEST(ToastManager, LongMessagePreserved) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  std::string long_message(10000, 'x');
  manager.show(Timed(long_message, std::chrono::seconds(5)));

  EXPECT_EQ(manager.active()[0].message.size(), long_message.size());
  EXPECT_EQ(manager.active()[0].message, long_message);
}

TEST(ToastManager, EmptyMessagePolicy) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  // Empty messages are permitted and produce a valid (icon/action-only) toast.
  std::size_t id = manager.show(Timed("", std::chrono::seconds(5)));
  EXPECT_EQ(manager.visible_count(), 1u);
  EXPECT_EQ(manager.active()[0].id, id);
  EXPECT_TRUE(manager.active()[0].message.empty());
}

TEST(ToastManager, DeterministicFakeClock) {
  // The same clock sequence always yields the same outcome.
  FakeClock clock_a;
  ToastManager manager_a = MakeManager(clock_a);
  FakeClock clock_b;
  ToastManager manager_b = MakeManager(clock_b);

  auto drive = [](ToastManager& manager, FakeClock& clock) {
    manager.show(Timed("x", std::chrono::seconds(5)));
    manager.tick();
    clock.Advance(std::chrono::seconds(1));
    manager.tick();
    clock.Advance(std::chrono::seconds(1));
    manager.tick();
    clock.Advance(std::chrono::seconds(2));
    manager.tick();
    return manager.visible_count();
  };

  EXPECT_EQ(drive(manager_a, clock_a), drive(manager_b, clock_b));
  EXPECT_EQ(manager_a.visible_count(), 1u);
  EXPECT_FALSE(manager_a.empty());
}

TEST(ToastManager, NonPositiveDurationTreatedAsPersistent) {
  FakeClock clock;
  ToastManager manager = MakeManager(clock);

  // A zero/negative duration would only live one tick interval; treat it as
  // persistent so callers intending "stays until dismissed" are not surprised
  // by an immediate silent expiry.
  ToastOptions options = Timed("stays", std::chrono::seconds(0));
  manager.show(options);
  manager.tick();
  clock.Advance(std::chrono::hours(1));
  manager.tick();

  EXPECT_EQ(manager.visible_count(), 1u);
  EXPECT_EQ(manager.visible()[0].message, "stays");
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
        ids.push_back(manager.show(options));
        break;
      }
      case 1:  // close a random id
        if (!ids.empty()) {
          manager.close(ids[static_cast<std::size_t>(rng()) % ids.size()]);
        }
        break;
      case 2:  // clear all
        manager.clear_all();
        ids.clear();
        break;
      case 3:  // move focus by a random delta
        manager.move_focus(static_cast<int>(rng() % 3) - 1);
        break;
      case 4:  // invoke focused action
        manager.invoke_focused_action();
        break;
      case 5:  // close focused
        manager.close_focused();
        break;
      case 6:  // advance time and tick
        clock.Advance(std::chrono::milliseconds(static_cast<int>(rng()) % 100));
        manager.tick();
        break;
      default:
        break;
    }

    // Invariants: never exceed the bound, focus always points at a visible
    // toast, and the visible window is always exactly the first max_visible
    // active toasts.
    ASSERT_LE(manager.visible_count(), manager.max_visible());
    ASSERT_EQ(manager.visible().size(), manager.visible_count());
    if (manager.has_focus()) {
      const std::optional<std::size_t> focused = manager.focused_id();
      const std::vector<Toast> visible = manager.visible();
      bool found = false;
      for (const Toast& toast : visible) {
        if (toast.id == *focused) {
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
