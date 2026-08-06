// CommandHistory — bounded, navigable command history for input and editor
// components.
//
// CommandHistory is a pure-data model with no dependency on FTXUI or any
// terminal backend. It owns every command it retains (std::string storage), so
// commands can safely outlive the strings they were added from (no retained
// string_view, pointer, or reference). It provides:
//
//   - bounded storage with a configurable capacity (0 disables retention),
//   - Up/Down-style Previous()/Next() navigation with deterministic boundary
//     behavior,
//   - substring and prefix search over the retained history,
//   - an optional persistence adapter, and
//   - an optional sensitive-command policy that stops selected commands from
//     being persisted.
//
// The class is not internally synchronized; callers that use it from multiple
// threads must provide their own synchronization.

#pragma once

#include <cstddef>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace terminal_ui_kit {
namespace command {

// Persistence adapter. Implementations persist a command history externally (a
// file, a database, in-memory store, ...) and must not throw. A failed Save()
// or Clear() is silently absorbed by CommandHistory and never mutates the
// in-memory history.
class CommandHistoryPersistence {
 public:
  virtual ~CommandHistoryPersistence() = default;

  // Persist `command`. Return true on success. On failure the in-memory
  // history is left untouched.
  virtual bool Save(const std::string& command) = 0;

  // Clear the external store. Defaults to success for adapters that have
  // nothing to clear.
  virtual bool Clear() { return true; }
};

// Decides whether a command is "sensitive", in which case CommandHistory keeps
// it in memory but never writes it to the persistence adapter.
class CommandSensitivePolicy {
 public:
  virtual ~CommandSensitivePolicy() = default;

  // Returns true when `command` must not be persisted.
  virtual bool IsSensitive(std::string_view command) const = 0;
};

// Default policy: no command is ever considered sensitive.
class NeverSensitivePolicy final : public CommandSensitivePolicy {
 public:
  bool IsSensitive(std::string_view /*command*/) const final { return false; }
};

// Policy that treats every command beginning with `prefix` as sensitive.
class PrefixSensitivePolicy final : public CommandSensitivePolicy {
 public:
  explicit PrefixSensitivePolicy(std::string prefix);

  bool IsSensitive(std::string_view command) const final;

 private:
  std::string prefix_;
};

class CommandHistory {
 public:
  // Constructs a history that retains at most `max_entries` commands, evicting
  // the oldest entry first when full. A `max_entries` of 0 disables retention:
  // Add() then ignores every command. `persistence` and `sensitive_policy` are
  // optional and may be swapped later via SetPersistence()/SetSensitivePolicy().
  explicit CommandHistory(std::size_t max_entries,
                          std::shared_ptr<CommandHistoryPersistence> persistence = nullptr,
                          std::shared_ptr<const CommandSensitivePolicy> sensitive_policy = nullptr);

  // Replaces the sensitive-command policy consulted by future Add() calls.
  void SetSensitivePolicy(std::shared_ptr<const CommandSensitivePolicy> policy);

  // Replaces the persistence adapter written to by future Add()/Clear() calls.
  void SetPersistence(std::shared_ptr<CommandHistoryPersistence> persistence);

  // Adds `command` to the history. Empty and whitespace-only commands are
  // ignored, as is a command identical to the most recently added one
  // (consecutive duplicates are deduplicated). The retained history is bounded
  // by the configured capacity, evicting the oldest entry when already full.
  // Submitting any command (including one that is ignored as blank or a
  // duplicate) resets the navigation cursor to the end, behind the newest
  // entry. If the command is sensitive per the active policy it is kept in
  // memory but not written to the persistence adapter. A persistence failure
  // never modifies the in-memory history.
  void Add(const std::string& command);

  // Returns the command one step towards the oldest entry and advances the
  // navigation cursor accordingly. At the oldest boundary the oldest entry is
  // returned again (the cursor never moves below it). On an empty history this
  // returns std::nullopt and does not change the cursor.
  std::optional<std::string> Previous();

  // Returns the command one step towards the newest entry and advances the
  // navigation cursor accordingly. Once the cursor has moved past the newest
  // entry (or was already there) this returns std::nullopt.
  std::optional<std::string> Next();

  // Returns every retained command containing `query` as a substring, in
  // chronological order (oldest first). An empty query matches every entry.
  std::vector<std::string> Search(std::string_view query) const;

  // Returns every retained command beginning with `prefix`, in chronological
  // order (oldest first). An empty prefix matches every entry.
  std::vector<std::string> SearchPrefix(std::string_view prefix) const;

  // Clears the retained history and resets the navigation cursor, then
  // forwards the clear to the persistence adapter. A persistence failure is
  // absorbed and never changes the (already emptied) in-memory history.
  void Clear();

  // Number of retained commands.
  std::size_t Size() const;

  // Configured capacity (the maximum number of commands that can be retained).
  std::size_t Capacity() const;

 private:
  bool IsBlank(std::string_view command) const;

  std::deque<std::string> entries_;
  std::size_t max_entries_;
  // Navigation cursor: an index into entries_, or entries_.size() to mean
  // "at the end, behind the newest entry" (the value after Add()/Clear()).
  std::size_t cursor_ = 0;
  std::shared_ptr<CommandHistoryPersistence> persistence_;
  std::shared_ptr<const CommandSensitivePolicy> sensitive_policy_;
};

inline PrefixSensitivePolicy::PrefixSensitivePolicy(std::string prefix)
    : prefix_(std::move(prefix)) {}

inline bool PrefixSensitivePolicy::IsSensitive(std::string_view command) const {
  return command.size() >= prefix_.size() && command.substr(0, prefix_.size()) == prefix_;
}

inline CommandHistory::CommandHistory(
    std::size_t max_entries, std::shared_ptr<CommandHistoryPersistence> persistence,
    std::shared_ptr<const CommandSensitivePolicy> sensitive_policy)
    : max_entries_(max_entries),
      persistence_(std::move(persistence)),
      sensitive_policy_(std::move(sensitive_policy)) {}

inline void CommandHistory::SetSensitivePolicy(
    std::shared_ptr<const CommandSensitivePolicy> policy) {
  sensitive_policy_ = std::move(policy);
}

inline void CommandHistory::SetPersistence(std::shared_ptr<CommandHistoryPersistence> persistence) {
  persistence_ = std::move(persistence);
}

inline bool CommandHistory::IsBlank(std::string_view command) const {
  return command.find_first_not_of(" \t\n\v\f\r") == std::string_view::npos;
}

inline void CommandHistory::Add(const std::string& command) {
  // A command identical to the most recently added one is not stored again, and
  // the configured capacity bounds retention (oldest evicted first). When the
  // capacity is 0 nothing is retained at all.
  // Submitting a command, even one that is ignored (blank or a consecutive
  // duplicate), returns navigation to the end (behind the newest entry) so an
  // unrelated Edit/Previous no longer picks up a mid-list position.
  const bool duplicate = !entries_.empty() && entries_.back() == command;
  if (IsBlank(command)) {
    cursor_ = entries_.size();
    return;
  }
  if (duplicate || max_entries_ == 0) {
    cursor_ = entries_.size();
    return;
  }
  if (entries_.size() == max_entries_) {
    entries_.pop_front();
  }
  entries_.push_back(command);
  cursor_ = entries_.size();

  // Persistence errors are absorbed here and never touch the in-memory history.
  // Sensitive commands are kept in memory but deliberately not persisted.
  const bool sensitive = sensitive_policy_ != nullptr && sensitive_policy_->IsSensitive(command);
  if (persistence_ != nullptr && !sensitive) {
    (void)persistence_->Save(command);
  }
}

inline std::optional<std::string> CommandHistory::Previous() {
  if (entries_.empty()) {
    return std::nullopt;
  }
  if (cursor_ != 0) {
    --cursor_;
  }
  return entries_[cursor_];
}

inline std::optional<std::string> CommandHistory::Next() {
  if (entries_.empty()) {
    return std::nullopt;
  }
  if (cursor_ == entries_.size()) {
    // Already at (or past) the newest entry; there is nothing newer to recall.
    return std::nullopt;
  }
  ++cursor_;
  if (cursor_ == entries_.size()) {
    // Just moved past the newest entry.
    return std::nullopt;
  }
  return entries_[cursor_];
}

inline std::vector<std::string> CommandHistory::Search(std::string_view query) const {
  std::vector<std::string> result;
  for (const std::string& entry : entries_) {
    if (entry.find(query) != std::string::npos) {
      result.push_back(entry);
    }
  }
  return result;
}

inline std::vector<std::string> CommandHistory::SearchPrefix(std::string_view prefix) const {
  std::vector<std::string> result;
  for (const std::string& entry : entries_) {
    if (entry.size() >= prefix.size() && entry.compare(0, prefix.size(), prefix) == 0) {
      result.push_back(entry);
    }
  }
  return result;
}

inline void CommandHistory::Clear() {
  entries_.clear();
  cursor_ = 0;
  if (persistence_ != nullptr) {
    (void)persistence_->Clear();
  }
}

inline std::size_t CommandHistory::Size() const { return entries_.size(); }

inline std::size_t CommandHistory::Capacity() const { return max_entries_; }

}  // namespace command
}  // namespace terminal_ui_kit