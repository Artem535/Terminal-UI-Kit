#include "terminal_ui_kit/components/toast_manager.h"

#include <algorithm>
#include <chrono>
#include <iterator>
#include <type_traits>
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

std::uint64_t ToastManager::Show(ToastOptions options) {
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

void ToastManager::Close(std::uint64_t id) {
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

void ToastManager::ClearAll() {
  visible_.clear();
  queue_.clear();
  focus_.reset();
}

void ToastManager::Update() {
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

std::optional<std::size_t> ToastManager::Focus() const { return focus_; }

void ToastManager::SetFocus(std::optional<std::size_t> index) {
  focus_.reset();
  if (index.has_value() && *index < visible_.size()) {
    focus_ = index;
  }
}

void ToastManager::MoveFocus(int delta) {
  if (visible_.empty()) {
    focus_.reset();
    return;
  }
  const std::size_t n = visible_.size();
  if (!focus_.has_value()) {
    focus_ = (delta > 0) ? std::size_t{0} : n - 1;
    return;
  }

  const std::size_t magnitude =
      static_cast<std::size_t>(static_cast<long long>(delta < 0 ? -delta : delta));
  const std::size_t step = magnitude % n == 0 ? n : magnitude % n;
  if (delta > 0) {
    focus_ = (*focus_ + step) % n;
  } else {
    focus_ = (*focus_ + n - (step % n)) % n;
  }
  if (focus_ == n) {
    focus_ = 0;
  }
}

std::optional<std::uint64_t> ToastManager::FocusedId() const {
  if (!focus_.has_value() || *focus_ >= visible_.size()) {
    return std::nullopt;
  }
  return visible_[*focus_].id;
}

void ToastManager::Invoke(std::uint64_t id) {
  for (std::size_t i = 0; i < visible_.size(); ++i) {
    if (visible_[i].id != id) {
      continue;
    }
    StoredToast& toast = visible_[i];
    if (!toast.action.has_value() || toast.action_invoked) {
      return;
    }
    toast.action_invoked = true;
    // Copy the callback and id before closing so the callback can freely add
    // or remove toasts without this method touching freed storage.
    std::function<void()> callback = toast.action->callback;
    Close(id);
    if (callback) {
      callback();
    }
    return;
  }
  // Queued toasts are not actionable until they become visible.
}

void ToastManager::InvokeFocused() {
  if (focus_.has_value() && *focus_ < visible_.size()) {
    Invoke(visible_[*focus_].id);
  }
}

std::vector<ToastInfo> ToastManager::Visible() const {
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

std::size_t ToastManager::VisibleCount() const { return visible_.size(); }

std::size_t ToastManager::QueuedCount() const { return queue_.size(); }

std::size_t ToastManager::MaxVisible() const { return max_visible_; }

bool ToastManager::Empty() const { return visible_.empty() && queue_.empty(); }

}  // namespace terminal_ui_kit