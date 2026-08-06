#include "terminal_ui_kit/components/toast_manager.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <utility>

namespace terminal_ui_kit {
namespace {

// Converts a size_t index into an iterator difference type without tripping
// -Wsign-conversion on the vector arithmetic.
std::ptrdiff_t Sel(const std::size_t index) {
  return static_cast<std::ptrdiff_t>(static_cast<long long>(index));
}

}  // namespace

std::chrono::steady_clock::time_point SystemToastClock::now() const {
  return std::chrono::steady_clock::now();
}

ToastManager::ToastManager(std::shared_ptr<const ToastClock> clock, std::size_t max_visible)
    : clock_(std::move(clock)), max_visible_(max_visible == 0 ? 1 : max_visible) {
  // A zero cap is nonsensical; clamp to one so the queue always has somewhere
  // to drain to.
}

ToastManager::StoredToast ToastManager::MakeStored(const ToastOptions& options) {
  StoredToast toast;
  toast.id = next_id_++;
  toast.message = options.message;
  toast.severity = options.severity;
  toast.timeout = options.timeout;
  toast.action = options.action;
  toast.remaining = toast.timeout.value_or(std::chrono::milliseconds(0));
  return toast;
}

std::uint64_t ToastManager::show(ToastOptions options) {
  StoredToast toast = MakeStored(options);
  const std::uint64_t id = toast.id;
  if (visible_.size() < max_visible_) {
    visible_.push_back(std::move(toast));
  } else {
    queue_.push_back(std::move(toast));
  }
  return id;
}

void ToastManager::PromoteFromQueue() {
  while (!queue_.empty() && visible_.size() < max_visible_) {
    visible_.push_back(std::move(queue_.front()));
    queue_.erase(queue_.begin());
  }
  if (focus_.has_value() && *focus_ >= visible_.size()) {
    focus_.reset();
  }
}

void ToastManager::SetFocusAfterRemoval(std::size_t removed_index) {
  if (!focus_.has_value()) {
    return;
  }
  if (*focus_ == removed_index) {
    focus_.reset();
  } else if (*focus_ > removed_index) {
    --*focus_;
  }
}

void ToastManager::close(std::uint64_t id) {
  for (std::size_t i = 0; i < visible_.size(); ++i) {
    if (visible_[i].id != id) {
      continue;
    }
    visible_.erase(visible_.begin() + Sel(i));
    SetFocusAfterRemoval(i);
    PromoteFromQueue();
    return;
  }
  for (std::size_t i = 0; i < queue_.size(); ++i) {
    if (queue_[i].id == id) {
      queue_.erase(queue_.begin() + Sel(i));
      return;
    }
  }
}

void ToastManager::clear_all() {
  visible_.clear();
  queue_.clear();
  focus_.reset();
}

void ToastManager::update() {
  if (!clock_) {
    return;  // A null clock is a no-op, never a crash (documented on the ctor).
  }
  const std::chrono::steady_clock::time_point now = clock_->now();
  if (!last_tick_.has_value()) {
    last_tick_ = now;
    PromoteFromQueue();
    return;
  }

  const std::chrono::steady_clock::duration elapsed = now - *last_tick_;
  last_tick_ = now;

  if (elapsed <= std::chrono::steady_clock::duration::zero()) {
    PromoteFromQueue();
    return;
  }

  // Expire timed visible toasts in id order. The focused toast's timeout is
  // paused, so it is skipped. Removal may shift later indices, so iterate by
  // index and only advance when the current slot survives.
  for (std::size_t i = 0; i < visible_.size();) {
    StoredToast& toast = visible_[i];
    const bool paused = focus_.has_value() && *focus_ == i;
    if (toast.persistent() || paused) {
      ++i;
      continue;
    }
    if (toast.remaining > elapsed) {
      toast.remaining -= elapsed;
      ++i;
      continue;
    }
    visible_.erase(visible_.begin() + Sel(i));
    SetFocusAfterRemoval(i);
    // Slot i is now the next toast (or past the end); do not advance.
  }

  PromoteFromQueue();
}

std::optional<std::size_t> ToastManager::focus() const { return focus_; }

void ToastManager::set_focus(std::optional<std::size_t> index) {
  focus_.reset();
  if (index.has_value() && *index < visible_.size()) {
    focus_ = index;
  }
}

void ToastManager::move_focus(int delta) {
  if (visible_.empty()) {
    focus_.reset();
    return;
  }
  const std::size_t n = visible_.size();
  if (!focus_.has_value()) {
    focus_ = (delta > 0) ? std::size_t{0} : n - 1;
    return;
  }

  // Compute the magnitude in a wide type so delta == INT_MIN can't overflow
  // when negated.
  const long long magnitude =
      delta < 0 ? -static_cast<long long>(delta) : static_cast<long long>(delta);
  const std::size_t step = magnitude % static_cast<long long>(n) == 0
                               ? n
                               : static_cast<std::size_t>(magnitude % static_cast<long long>(n));
  if (delta > 0) {
    focus_ = (*focus_ + step) % n;
  } else {
    focus_ = (*focus_ + n - (step % n)) % n;
  }
}

std::optional<std::uint64_t> ToastManager::focused_id() const {
  if (!focus_.has_value() || *focus_ >= visible_.size()) {
    return std::nullopt;
  }
  return visible_[*focus_].id;
}

bool ToastManager::invoke(std::uint64_t id) {
  for (std::size_t i = 0; i < visible_.size(); ++i) {
    if (visible_[i].id != id) {
      continue;
    }
    StoredToast& toast = visible_[i];
    if (!toast.action.has_value() || toast.action_invoked) {
      return false;
    }
    toast.action_invoked = true;
    // Copy the callback and id before closing so the callback can freely add
    // or remove toasts without this method touching freed storage.
    std::function<void()> callback = toast.action->callback;
    close(id);
    if (callback) {
      callback();
    }
    return true;
  }
  // Queued toasts are not actionable until they become visible.
  return false;
}

bool ToastManager::invoke_focused() {
  if (focus_.has_value() && *focus_ < visible_.size()) {
    return invoke(visible_[*focus_].id);
  }
  return false;
}

std::vector<ToastInfo> ToastManager::visible() const {
  std::vector<ToastInfo> result;
  result.reserve(visible_.size());
  for (std::size_t i = 0; i < visible_.size(); ++i) {
    const StoredToast& toast = visible_[i];
    ToastInfo info;
    info.id = toast.id;
    info.message = toast.message;
    info.severity = toast.severity;
    info.persistent = toast.persistent();
    info.focused = focus_.has_value() && *focus_ == i;
    info.action_label = toast.action.has_value() ? toast.action->label : std::string();
    info.action_invoked = toast.action_invoked;
    info.remaining = toast.remaining;
    result.push_back(std::move(info));
  }
  return result;
}

std::size_t ToastManager::visible_count() const { return visible_.size(); }

std::size_t ToastManager::queued_count() const { return queue_.size(); }

std::size_t ToastManager::max_visible() const { return max_visible_; }

bool ToastManager::empty() const { return visible_.empty() && queue_.empty(); }

}  // namespace terminal_ui_kit