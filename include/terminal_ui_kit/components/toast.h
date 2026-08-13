#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

// Severity of a toast notification (PRD section 40). Each severity maps to a
// distinct icon and Theme role in the view; the icon carries meaning even when
// color is disabled.
enum class ToastSeverity {
  kInfo,
  kSuccess,
  kWarning,
  kError,
};

// The action a toast can expose. Callers supply a semantic label and a
// callback; the manager owns (and safely invokes) the callback.
struct ToastAction {
  std::string label;
  std::function<void()> callback;
};

// The clock abstraction injected into ToastManager so expiry, pause and resume
// are deterministic in tests (no real sleeps). Defaults to steady_clock::now.
using ToastClock = std::function<std::chrono::steady_clock::time_point()>;

// Everything needed to create one toast.
struct ToastOptions {
  std::string message;
  ToastSeverity severity = ToastSeverity::kInfo;
  // How long the toast stays visible (remaining time). std::nullopt means it
  // never expires (a persistent toast). An empty message is permitted and
  // renders as an icon/action-only toast.
  std::optional<std::chrono::steady_clock::duration> duration = std::nullopt;
  std::optional<ToastAction> action = std::nullopt;
};

// Construction/configuration options for a ToastManager.
struct ToastManagerOptions {
  // Maximum number of toasts rendered at once. Additional shown toasts are
  // queued (FIFO by insertion id) and become visible as earlier ones are
  // removed. Must be >= 1 (clamped to 1 if 0 is passed).
  std::size_t max_visible = 5;
  ToastClock clock = [] { return std::chrono::steady_clock::now(); };
};

// One active toast as exposed to views/consumers. The manager owns the
// backing storage; callers must not retain references beyond the owning
// manager's lifetime (safe owning types only).
struct Toast {
  // Stable unique id, monotonically increasing per manager.
  std::size_t id = 0;
  std::string message;
  ToastSeverity severity = ToastSeverity::kInfo;
  // Remaining visible time; std::nullopt => persistent.
  std::optional<std::chrono::steady_clock::duration> remaining = std::nullopt;
  std::optional<ToastAction> action = std::nullopt;
};

// The retained state and timing logic of the toast system. Independent of any
// rendering backend so it can be unit-tested directly with an injected fake
// clock. Views (ToastView) read from visible()/active() and forward input and
// time ticks here.
//
// Ordering: toasts are kept in FIFO insertion order. active() returns all of
// them (visible + queued); visible() returns only the first max_visible
// (the window actually rendered). The rest are queued and slide into the
// visible window as earlier toasts are removed.
class ToastManager {
 public:
  explicit ToastManager(ToastManagerOptions options = {});

  // Adds a toast and returns its stable id. Appended at the end of the queue.
  std::size_t show(const ToastOptions& options);

  // Removes a toast by id (no-op if absent). Focus stays valid: the selection
  // is clamped to the remaining visible window.
  void close(std::size_t id);

  // Removes every active toast (visible and queued) and clears focus.
  void clear_all();

  // Advances time by the clock difference since the previous tick, expiring
  // and removing non-persistent toasts whose remaining time has elapsed.
  // While any toast is focused (focused_id() is set) time is frozen: remaining
  // durations are untouched, so timeouts pause and resume around focus.
  void tick();

  // ---- Focus / selection -------------------------------------------------
  // A focused toast is always within the visible window. While one is focused,
  // timeout is paused (see tick()).
  bool set_focused(std::size_t id);  // false if id is not visible
  void clear_focus();
  bool has_focus() const;
  std::optional<std::size_t> focused_id() const;

  // Moves selection among visible toasts. Stepping past the last (delta>0) or
  // before the first (delta<0) clear the selection. Returns true if the focus
  // state changed.
  bool move_focus(int delta);

  // Invokes the focused toast's action exactly once and removes that toast.
  // Safe if the callback itself removes toasts (the callback is copied before
  // invocation; removal is done beforehand so the callback sees consistent
  // state). Returns true if an action was invoked. Focus stays on the toast
  // that replaces the removed one (or is cleared if none remains).
  bool invoke_focused_action();

  // Removes the focused toast (manual close). Returns true if one was removed.
  bool close_focused();

  // ---- Queries -----------------------------------------------------------
  // All active toasts (visible + queued), oldest first.
  const std::vector<Toast>& active() const { return toasts_; }
  // Only the visible window (first max_visible active toasts), oldest first.
  std::vector<Toast> visible() const;
  std::size_t visible_count() const { return std::min(max_visible_, toasts_.size()); }
  std::size_t queued_count() const { return toasts_.size() - visible_count(); }
  bool empty() const { return toasts_.empty(); }
  std::size_t max_visible() const { return max_visible_; }
  std::size_t next_id() const { return next_id_; }

 private:
  void ClampFocus();
  void OnTimeElapsed(std::chrono::steady_clock::duration elapsed);

  std::size_t max_visible_;
  ToastClock clock_;
  std::vector<Toast> toasts_;
  std::optional<std::size_t> focused_;  // position into toasts_
  std::size_t next_id_ = 0;
  std::optional<std::chrono::steady_clock::time_point> last_tick_;
};

// Rendering/behaviour options for ToastView.
struct ToastViewOptions {
  // Render toasts without color, relying on severity icons and bold instead of
  // the theme's color roles (no-color fallback for terminals without color).
  bool no_color = false;
  // Maximum width (cells) of a single toast before its message wraps.
  std::size_t max_toast_width = 60;
};

// FTXUI view over a ToastManager. Renders the visible toasts (styled by
// severity, with the focused toast highlighted) and translates keyboard input
// into manager calls: Tab/Shift+Tab move focus, Enter invokes the focused
// action, Delete/Backspace close the focused toast. It drives expiry by
// hooking FTXUI's animation ticks (calling manager.tick()), so it requires no
// event loop or background thread of its own. It only requests animation
// frames while there is at least one toast, so an empty manager stays idle.
//
// Note on focus: a toast is "focused" when selected via Tab; while any toast
// is selected the manager pauses timeouts (see ToastManager::tick).
//
// Lifetime: ToastView references the ToastManager without owning it; the
// manager must outlive the view.
class ToastView : public ftxui::ComponentBase {
 public:
  ToastView(ToastManager& manager, const Theme& theme, ToastViewOptions options = {});

  // Toggles the no-color fallback at runtime, re-styling on the next frame.
  // Lets interactive demos switch between color and no-color modes live.
  void set_no_color(bool no_color);

 private:
  ftxui::Element Render() override;
  bool OnEvent(ftxui::Event event) override;
  bool Focusable() const override { return true; }
  void OnAnimation(ftxui::animation::Params& params) override;

  void RebuildTheme();

  ToastManager& manager_;
  Theme base_theme_;  // as supplied (colored)
  Theme theme_;       // effective (color-stripped when no_color is set)
  ToastViewOptions options_;
};

}  // namespace terminal_ui_kit
