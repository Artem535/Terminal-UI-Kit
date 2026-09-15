#include "terminal_ui_kit/editor/command_history.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

namespace terminal_ui_kit {
namespace editor {
namespace {

// Returns true when `s` is empty or made only of whitespace characters.
bool is_blank(std::string_view s) {
  return std::all_of(s.begin(), s.end(),
                     [](char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; });
}

}  // namespace

SensitiveCommandPolicy::SensitiveCommandPolicy(std::vector<std::string> sensitive)
    : sensitive_(std::move(sensitive)) {}

bool SensitiveCommandPolicy::ShouldPersist(std::string_view command) const {
  return std::find(sensitive_.begin(), sensitive_.end(), command) == sensitive_.end();
}

CommandHistory::CommandHistory(std::size_t max_entries)
    : max_entries_(max_entries), cursor_(std::nullopt) {}

void CommandHistory::SetPersistenceStore(std::unique_ptr<CommandHistoryStore> store) {
  store_ = std::move(store);
}

void CommandHistory::SetPersistencePolicy(std::unique_ptr<CommandPersistencePolicy> policy) {
  policy_ = std::move(policy);
}

void CommandHistory::Add(std::string command) {
  if (max_entries_ == 0) {
    // Capacity 0 stores nothing; keep the cursor at the draft position.
    return;
  }
  if (is_blank(command)) {
    // Empty and whitespace-only commands are ignored.
    return;
  }
  if (!entries_.empty() && entries_.back() == command) {
    // Consecutive duplicates are ignored.
    return;
  }
  if (entries_.size() == max_entries_) {
    // Bounded storage: evict the oldest entry to make room. A deque makes
    // this O(1) at capacity rather than O(n) per add on a shifted vector.
    entries_.pop_front();
  }
  entries_.push_back(std::move(command));
  // Adding a new command resets the navigation cursor.
  cursor_ = std::nullopt;

  if (store_ != nullptr) {
    try {
      if (policy_ == nullptr || policy_->ShouldPersist(entries_.back())) {
        store_->Persist(entries_.back());
      }
    } catch (...) {
      // A failing store (or policy) must never corrupt the in-memory history.
    }
  }
}

std::optional<std::string> CommandHistory::Previous() {
  if (entries_.empty()) return std::nullopt;
  if (!cursor_.has_value()) {
    cursor_ = entries_.size() - 1;  // Start at the most recent command.
  } else if (*cursor_ > 0) {
    --(*cursor_);
  }
  // Else cursor_ == 0: stay clamped at the oldest entry.
  return entries_[*cursor_];
}

std::optional<std::string> CommandHistory::Next() {
  if (entries_.empty()) return std::nullopt;
  if (!cursor_.has_value()) {
    // Already at the draft position; there is nothing newer.
    return std::nullopt;
  }
  if (*cursor_ == entries_.size() - 1) {
    // At the most recent entry; step back to the draft position.
    cursor_ = std::nullopt;
    return std::nullopt;
  }
  ++(*cursor_);
  return entries_[*cursor_];
}

std::vector<std::string> CommandHistory::Search(std::string_view query) const {
  std::vector<std::string> results;
  for (auto it = entries_.rbegin(); it != entries_.rend(); ++it) {
    if (it->find(query) != std::string::npos) results.push_back(*it);
  }
  return results;
}

std::vector<std::string> CommandHistory::SearchPrefix(std::string_view prefix) const {
  std::vector<std::string> results;
  for (auto it = entries_.rbegin(); it != entries_.rend(); ++it) {
    if (it->starts_with(prefix)) results.push_back(*it);
  }
  return results;
}

void CommandHistory::Clear() {
  entries_.clear();
  cursor_ = std::nullopt;
}

std::size_t CommandHistory::Size() const { return entries_.size(); }

std::size_t CommandHistory::Capacity() const { return max_entries_; }

std::optional<std::string> CommandHistory::Current() const {
  if (!cursor_.has_value()) return std::nullopt;
  return entries_[*cursor_];
}

}  // namespace editor
}  // namespace terminal_ui_kit

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>

#include "terminal_ui_kit/editor/command_history.h"

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
