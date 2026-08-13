#pragma once

#include <cstddef>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace terminal_ui_kit {
namespace editor {

// Decides whether a command that was just accepted into the in-memory history
// should also be forwarded to the persistence store. Return false to suppress
// persistence for a command without removing it from in-memory history.
class CommandPersistencePolicy {
 public:
  virtual ~CommandPersistencePolicy() = default;
  virtual bool ShouldPersist(std::string_view command) const = 0;
};

// Default policy: persist every accepted command.
class AlwaysPersistPolicy : public CommandPersistencePolicy {
 public:
  bool ShouldPersist(std::string_view) const override { return true; }
};

// Blocks an exact, case-sensitive set of commands from being persisted. The
// blocked commands are still stored in the in-memory history; only their
// persistence is suppressed. A command is sensitive when it matches one of
// the configured commands verbatim (no trimming, no prefix matching).
class SensitiveCommandPolicy : public CommandPersistencePolicy {
 public:
  explicit SensitiveCommandPolicy(std::vector<std::string> sensitive);
  bool ShouldPersist(std::string_view command) const override;

 private:
  std::vector<std::string> sensitive_;
};

// Optional persistence abstraction for a CommandHistory. CommandHistory calls
// Persist() best-effort after a command has been accepted into in-memory
// history; any exception thrown by a store (or by a policy consulted on the
// way to it) is swallowed so that a failing store never corrupts the
// in-memory history. Thread-safety, if required, is the store's own
// responsibility.
class CommandHistoryStore {
 public:
  virtual ~CommandHistoryStore() = default;
  virtual void Persist(const std::string& command) = 0;
};

// A bounded, navigable command history for input and editor components.
//
// Commands are retained as owning std::string values, newest last. Storage is
// bounded by a configurable maximum: adding a command once that maximum is
// reached evicts the oldest entry. Empty and whitespace-only commands are
// ignored, as are consecutive duplicates (a command equal to the most recent
// one). Non-consecutive duplicates are stored normally.
//
// Navigation cursor: Previous() walks from the most recent command toward the
// oldest and stays clamped at the first entry; Next() walks back toward the
// most recent and, once already at the most recent entry, returns to the
// "draft" position (nullopt). Adding a command always resets the cursor.
//
// Search results (Search / SearchPrefix) come back in a stable order: most
// recent command first, oldest last.
//
// The model has no FTXUI dependency, so pure-data consumers and tests can use
// it without a terminal backend.
class CommandHistory {
 public:
  // `max_entries` is the storage bound. A capacity of 0 stores nothing:
  // Add() is a safe no-op, Size() is always 0, navigation returns nullopt and
  // searches return empty results.
  explicit CommandHistory(std::size_t max_entries);

  CommandHistory(const CommandHistory&) = delete;
  CommandHistory& operator=(const CommandHistory&) = delete;
  CommandHistory(CommandHistory&&) noexcept = default;
  CommandHistory& operator=(CommandHistory&&) noexcept = default;

  // Attach an optional persistence sink. Passing nullptr detaches it.
  void SetPersistenceStore(std::unique_ptr<CommandHistoryStore> store);
  // Attach an optional persistence policy. Passing nullptr restores the
  // default behavior (persist every accepted command).
  void SetPersistencePolicy(std::unique_ptr<CommandPersistencePolicy> policy);

  // Adds a command to history and resets the navigation cursor. Empty,
  // whitespace-only and consecutive-duplicate commands are ignored (ignored
  // commands leave the navigation cursor unchanged). When the store and policy
  // allow, the accepted command is also persisted. Storage is bounded: adding
  // at capacity evicts the oldest command.
  void Add(std::string command);

  // Moves the cursor one step toward the oldest command and returns it, or
  // std::nullopt when history is empty. Boundary: stays at the oldest entry.
  std::optional<std::string> Previous();

  // Moves the cursor one step toward the most recent command and returns it.
  // Once the cursor is at the most recent command, a further call returns to
  // the draft position and returns std::nullopt.
  std::optional<std::string> Next();

  // Returns all commands whose text contains `query` as a substring, most
  // recent first. An empty query matches every command.
  std::vector<std::string> Search(std::string_view query) const;

  // Returns all commands that begin with `prefix`, most recent first. An
  // empty prefix matches every command.
  std::vector<std::string> SearchPrefix(std::string_view prefix) const;

  // Empties history and resets the navigation cursor.
  void Clear();

  [[nodiscard]] std::size_t Size() const;
  [[nodiscard]] std::size_t Capacity() const;

  // The command the navigation cursor is currently pointed at, or nullopt
  // when the cursor is at the draft (top) position or history is empty.
  [[nodiscard]] std::optional<std::string> Current() const;

 private:
  std::size_t max_entries_;
  std::deque<std::string> entries_;
  std::optional<std::size_t> cursor_;
  std::unique_ptr<CommandHistoryStore> store_;
  std::unique_ptr<CommandPersistencePolicy> policy_;
};

}  // namespace editor
}  // namespace terminal_ui_kit
