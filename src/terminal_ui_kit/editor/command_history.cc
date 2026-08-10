#include "terminal_ui_kit/editor/command_history.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>

namespace terminal_ui_kit {

CommandHistory::CommandHistory(std::size_t max_entries) : max_entries_(max_entries) {}

void CommandHistory::add(std::string entry) {
  // Any submission exits navigation (and clears the draft), including blank
  // or duplicate entries.
  end_navigation();
  // Skip blank and whitespace-only submissions.
  const bool blank = std::all_of(entry.begin(), entry.end(), [](unsigned char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
  });
  if (blank) {
    return;
  }
  if (!entries_.empty() && entries_.back() == entry) {
    return;
  }
  if (max_entries_ == 0) {
    return;
  }
  if (entries_.size() >= max_entries_) {
    entries_.pop_front();
  }
  entries_.push_back(std::move(entry));
}

void CommandHistory::clear() {
  entries_.clear();
  end_navigation();
}

std::size_t CommandHistory::size() const { return entries_.size(); }

bool CommandHistory::empty() const { return entries_.empty(); }

const std::string& CommandHistory::at(std::size_t index) const { return entries_.at(index); }

const std::deque<std::string>& CommandHistory::entries() const { return entries_; }

void CommandHistory::set_draft(std::string draft) { draft_ = std::move(draft); }

bool CommandHistory::navigating() const { return nav_index_.has_value(); }

bool CommandHistory::previous(std::string& out) {
  if (entries_.empty()) {
    return false;
  }
  if (!nav_index_) {
    nav_index_ = entries_.size() - 1;
  } else if (*nav_index_ > 0) {
    --(*nav_index_);
  }
  out = entries_[*nav_index_];
  return true;
}

bool CommandHistory::next(std::string& out) {
  if (!nav_index_) {
    return false;
  }
  if (*nav_index_ + 1 < entries_.size()) {
    ++(*nav_index_);
    out = entries_[*nav_index_];
    return true;
  }
  // Newest reached: restore the draft and exit navigation mode.
  out = draft_;
  nav_index_.reset();
  draft_.clear();
  return true;
}

void CommandHistory::end_navigation() {
  nav_index_.reset();
  draft_.clear();
}

}  // namespace terminal_ui_kit
