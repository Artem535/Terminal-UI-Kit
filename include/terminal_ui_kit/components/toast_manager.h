#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace terminal_ui_kit {

// Semantic severity of a toast (PRD section 40).
enum class ToastSeverity {
  kInfo,
  kSuccess,
  kWarning,
  kError,
};

// An action associated with a toast (PRD section 40, "actions"). The callback
// must be cheap, must not retain the manager by reference, and may add/remove
// toasts while it runs (see ToastManager::Invoke). It is invoked at most once.
struct ToastAction {
  std::string label;
  std::function<void()> callback;
};

// Immutable description used to enqueue a toast. A `nullopt` timeout makes the
// toast persistent: it only goes away on manual close or clear-all.
struct ToastOptions {
  std::string message;
  ToastSeverity severity = ToastSeverity::kInfo;
  std::optional<std::chrono::milliseconds> timeout;
  std::optional<ToastAction> action;
};

// Read-only snapshot of one visible toast, rebuilt on demand for rendering and
// tests. Contains no references into the manager's internal storage.
struct ToastInfo {
  std::uint64_t id;
  std::string message;
  ToastSeverity severity;
  bool persistent = true;
  bool focused = false;
  std::string action_label;  // empty when the toast has no action
  bool action_invoked = false;
  std::chrono::steady_clock::duration remaining{};  // seconds left for timed toasts
};

// Abstract time source so tests can drive expiry deterministically without
// waiting on a real clock (PRD section 40 architecture requirement).
class ToastClock {
 public:
  virtual ~ToastClock() = default;
  [[nodiscard]] virtual std::chrono::steady_clock::time_point now() const = 0;
};

// The default clock backed by std::chrono::steady_clock.
class SystemToastClock final : public ToastClock {
 public:
  [[nodiscard]] std::chrono::steady_clock::time_point now() const override;
};

// Owns the retained state of a toast tray: the visible toasts, the FIFO queue
// of overflow toasts, the injected clock, and the current focus. This class is
// intentionally free of FTXUI types so it can be unit-tested and embedded on
// its own. Rendering is a separate concern (see components/toast_view.h).
//
// Ordering is deterministic: toasts are identified by a monotonically
// increasing id; visible toasts keep insertion order; when a slot frees up the
// longest-waiting queued toast is promoted into it. Timed toasts expire oldest
// (lowest id) first when their remaining time elapses. The focused toast's
// timeout is paused while it stays focused. Time spent in the queue does not
// count toward a toast's timeout: its countdown starts when it becomes visible,
// so a timed toast queued behind a persistent one only starts ticking once a
// slot frees up.
class ToastManager {
 public:
  // `clock` must outlive the manager and must not be null (a null clock turns
  // `update()` into a no-op rather than crashing); `max_visible` caps how many
  // toasts are shown at once (extra toasts wait in the queue).
  explicit ToastManager(std::shared_ptr<const ToastClock> clock, std::size_t max_visible = 5);

  // Adds a toast, returning its stable id. The toast is shown immediately if a
  // visible slot is free, otherwise it waits in the queue.
  std::uint64_t show(ToastOptions options);

  // Removes a visible or queued toast by id. No-op when the id is unknown.
  void close(std::uint64_t id);

  // Removes every toast and clears focus.
  void clear_all();

  // Advances time by the injected clock's delta and applies expirations and
  // queue promotion. Call each frame (the toast view does this on render).
  void update();

  // --- focus ----------------------------------------------------------------
  // focus is an index into the visible list; std::nullopt means no toast is
  // focused. All focus operations keep the index valid (see close/update).

  std::optional<std::size_t> focus() const;
  void set_focus(std::optional<std::size_t> index);
  // Moves focus by `delta` positions, wrapping around the visible toasts. With
  // no focus set, enters at the first toast (delta > 0) or last (delta < 0).
  void move_focus(int delta);
  [[nodiscard]] std::optional<std::uint64_t> focused_id() const;

  // Invokes the focused toast's action (at most once) and closes it, returning
  // true when an action was actually invoked (false when unfocused or the
  // focused toast has no action).
  bool invoke_focused();
  // Invokes the toast's action with `id` (at most once) and closes it,
  // returning true when an action was invoked. Returns false when the id is
  // unknown, the toast has no action, or its action was already invoked. A
  // toast without an action is not closed by this call; use `close` instead.
  bool invoke(std::uint64_t id);

  // --- read-only access ------------------------------------------------------
  [[nodiscard]] std::vector<ToastInfo> visible() const;
  [[nodiscard]] std::size_t visible_count() const;
  [[nodiscard]] std::size_t queued_count() const;
  [[nodiscard]] std::size_t max_visible() const;
  [[nodiscard]] bool empty() const;

 private:
  struct StoredToast {
    std::uint64_t id = 0;
    std::string message;
    ToastSeverity severity = ToastSeverity::kInfo;
    std::optional<std::chrono::milliseconds> timeout;
    std::optional<ToastAction> action;
    std::chrono::steady_clock::duration remaining{};
    bool action_invoked = false;
    bool persistent() const { return !timeout.has_value(); }
  };

  StoredToast MakeStored(const ToastOptions& options);
  void PromoteFromQueue();
  void SetFocusAfterRemoval(std::size_t removed_index);

  std::shared_ptr<const ToastClock> clock_;
  std::size_t max_visible_;
  std::vector<StoredToast> visible_;
  std::vector<StoredToast> queue_;
  std::optional<std::size_t> focus_;
  std::optional<std::chrono::steady_clock::time_point> last_tick_;
  std::uint64_t next_id_ = 1;
};

}  // namespace terminal_ui_kit