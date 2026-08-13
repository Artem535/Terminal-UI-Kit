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

void CommandHistory::SetPersistentStore(std::unique_ptr<CommandHistoryStore> store) {
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
    // Bounded storage: evict the oldest entry to make room.
    entries_.erase(entries_.begin());
  }
  entries_.push_back(std::move(command));
  // Adding a new command resets the navigation cursor.
  cursor_ = std::nullopt;

  if (store_ != nullptr && (policy_ == nullptr || policy_->ShouldPersist(entries_.back()))) {
    try {
      store_->Persist(entries_.back());
    } catch (...) {
      // A failing store must never corrupt the in-memory history.
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
