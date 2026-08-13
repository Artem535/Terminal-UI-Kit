#include "terminal_ui_kit/components/toast.h"

#include <algorithm>
#include <utility>

#include <ftxui/component/animation.hpp>
#include <ftxui/component/event.hpp>

#include "terminal_ui_kit/components/style_bridge.h"

namespace terminal_ui_kit {

namespace {

// Minimum elapsed resolution below which we do not bother advancing remaining
// time. Mirrors the fact that a zero/negative tick is a no-op.
using Duration = std::chrono::steady_clock::duration;

}  // namespace

ToastManager::ToastManager(ToastManagerOptions options)
    : options_(options),
      max_visible_(std::max<std::size_t>(1, options.max_visible)),
      clock_(options.clock ? std::move(options.clock)
                           : ([] { return std::chrono::steady_clock::now(); })) {}

std::size_t ToastManager::Show(const ToastOptions& options) {
  const std::size_t id = next_id_++;
  Toast toast;
  toast.id = id;
  toast.message = options.message;
  toast.severity = options.severity;
  toast.remaining = options.duration;
  toast.action = options.action;
  toasts_.push_back(std::move(toast));
  return id;
}

void ToastManager::Close(std::size_t id) {
  const auto it = std::find_if(toasts_.begin(), toasts_.end(),
                               [id](const Toast& toast) { return toast.id == id; });
  if (it == toasts_.end()) {
    return;
  }
  toasts_.erase(it);
  ClampFocus();
}

void ToastManager::ClearAll() {
  toasts_.clear();
  focused_.reset();
}

std::size_t ToastManager::VisibleIndexToPosition(std::size_t visible_index) const {
  // Visible toasts occupy positions [0, visible_count) in insertion order.
  return std::min(visible_index, visible_count());
}

void ToastManager::ClampFocus() {
  if (!focused_) {
    return;
  }
  const std::size_t count = visible_count();
  if (count == 0) {
    focused_.reset();
    return;
  }
  if (*focused_ >= count) {
    focused_ = count - 1;
  }
}

bool ToastManager::SetFocused(std::size_t id) {
  if (toasts_.empty()) {
    return false;
  }
  const std::size_t count = visible_count();
  for (std::size_t index = 0; index < count; ++index) {
    if (toasts_[index].id == id) {
      focused_ = index;
      return true;
    }
  }
  return false;
}

void ToastManager::ClearFocus() { focused_.reset(); }

bool ToastManager::HasFocus() const { return focused_.has_value(); }

std::optional<std::size_t> ToastManager::FocusedId() const {
  if (!focused_) {
    return std::nullopt;
  }
  return toasts_[*focused_].id;
}

bool ToastManager::MoveFocus(int delta) {
  const std::size_t count = visible_count();
  if (count == 0) {
    return false;
  }
  if (!focused_) {
    // Nothing selected: select the first visible for delta>=0, last for delta<0.
    focused_ = delta >= 0 ? 0 : count - 1;
    return true;
  }
  const std::size_t current = *focused_;
  if (delta > 0) {
    if (current + 1 >= count) {
      // Past the last visible toast: clear selection (timeouts resume).
      focused_.reset();
      return true;
    }
    focused_ = current + 1;
    return true;
  }
  if (delta < 0) {
    if (current == 0) {
      focused_.reset();
      return true;
    }
    focused_ = current - 1;
    return true;
  }
  return false;
}

bool ToastManager::InvokeFocusedAction() {
  if (!focused_ || *focused_ >= toasts_.size()) {
    return false;
  }
  const Toast& toast = toasts_[*focused_];
  if (!toast.action) {
    return false;
  }
  // Copy the callback before removing the toast so the manager is in a
  // consistent state when the callback runs, and so a callback that itself
  // removes toasts cannot invalidate anything we hold.
  std::function<void()> callback = toast.action->callback;
  const std::size_t position = *focused_;
  toasts_.erase(toasts_.begin() + static_cast<std::ptrdiff_t>(position));
  ClampFocus();
  if (callback) {
    callback();
  }
  return true;
}

bool ToastManager::CloseFocused() {
  if (!focused_ || *focused_ >= toasts_.size()) {
    return false;
  }
  const std::size_t position = *focused_;
  toasts_.erase(toasts_.begin() + static_cast<std::ptrdiff_t>(position));
  ClampFocus();
  return true;
}

void ToastManager::OnTimeElapsed(Duration elapsed) {
  if (elapsed <= Duration::zero()) {
    return;
  }
  for (auto it = toasts_.begin(); it != toasts_.end();) {
    bool remove = false;
    if (it->remaining) {
      *it->remaining -= elapsed;
      if (*it->remaining <= Duration::zero()) {
        remove = true;
      }
    }
    if (remove) {
      // If the removed toast was before or at the focus position, ClampFocus
      // after the loop keeps the selection valid.
      it = toasts_.erase(it);
    } else {
      ++it;
    }
  }
  ClampFocus();
}

void ToastManager::Tick() {
  const auto now = (clock_ ? clock_() : std::chrono::steady_clock::now());
  if (!last_tick_) {
    last_tick_ = now;
    return;
  }
  const Duration elapsed = now - *last_tick_;
  last_tick_ = now;
  if (focused_) {
    // Timeout is paused while a toast is focused: the clock still advances so
    // that unfocusing resumes from the correct point, but no remaining time is
    // consumed.
    return;
  }
  OnTimeElapsed(elapsed);
}

ToastView::ToastView(ToastManager& manager, const Theme& theme, ToastViewOptions options)
    : manager_(manager), base_theme_(theme), options_(options) {
  RebuildTheme();
}

void ToastView::SetNoColor(bool no_color) {
  options_.no_color = no_color;
  RebuildTheme();
}

void ToastView::RebuildTheme() {
  theme_ = options_.no_color ? without_color(base_theme_) : base_theme_;
}

ftxui::Element ToastView::Render() {
  ftxui::animation::RequestAnimationFrame();

  const std::size_t count = manager_.visible_count();
  if (count == 0) {
    return ftxui::text("");
  }

  const auto visible = manager_.Visible();
  std::size_t focused_pos = static_cast<std::size_t>(-1);
  if (manager_.HasFocus()) {
    const std::optional<std::size_t> id = manager_.FocusedId();
    for (std::size_t i = 0; i < visible.size() && i < count; ++i) {
      if (visible[i].id == *id) {
        focused_pos = i;
        break;
      }
    }
  }

  ftxui::Elements rows;
  for (std::size_t index = 0; index < count; ++index) {
    const Toast& toast = visible[index];
    const bool focused = (index == focused_pos);

    const char* icon = "i";
    TextStyle role = theme_.primary;
    switch (toast.severity) {
      case ToastSeverity::kInfo:
        icon = "ℹ";
        role = theme_.primary;
        break;
      case ToastSeverity::kSuccess:
        icon = "✓";
        role = theme_.success;
        break;
      case ToastSeverity::kWarning:
        icon = "▲";
        role = theme_.warning;
        break;
      case ToastSeverity::kError:
        icon = "✗";
        role = theme_.error;
        break;
    }

    ftxui::Element icon_element = ftxui::text(icon) | to_decorator(role);
    ftxui::Element message_element = toast.message.empty()
                                         ? ftxui::text("")
                                         : ftxui::paragraph(toast.message) | to_decorator(role);

    ftxui::Element row =
        ftxui::hbox({std::move(icon_element), ftxui::text(" "), std::move(message_element)});

    if (toast.action) {
      row = ftxui::hbox({std::move(row), ftxui::text("  [") | ftxui::dim,
                         ftxui::text(toast.action->label) | to_decorator(theme_.accent),
                         ftxui::text("]") | ftxui::dim});
    }

    ftxui::Element wrapped;
    if (focused) {
      wrapped = ftxui::hbox({ftxui::text("▶ "), std::move(row)}) | ftxui::inverted;
    } else {
      wrapped = ftxui::hbox({ftxui::text("  "), std::move(row)});
    }
    rows.push_back(std::move(wrapped));
  }
  return ftxui::vbox(std::move(rows)) |
         ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, static_cast<int>(options_.max_toast_width));
}

bool ToastView::OnEvent(ftxui::Event event) {
  if (event == ftxui::Event::Tab) {
    return manager_.MoveFocus(1);
  }
  if (event == ftxui::Event::TabReverse) {
    return manager_.MoveFocus(-1);
  }
  if (event == ftxui::Event::Return) {
    return manager_.InvokeFocusedAction();
  }
  if (event == ftxui::Event::Delete || event == ftxui::Event::Backspace) {
    return manager_.CloseFocused();
  }
  return false;
}

void ToastView::OnAnimation(ftxui::animation::Params& params) {
  (void)params;
  manager_.Tick();
  ftxui::animation::RequestAnimationFrame();
}

}  // namespace terminal_ui_kit
