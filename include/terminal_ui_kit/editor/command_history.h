#pragma once

#include <cstddef>
#include <deque>
#include <optional>
#include <string>

namespace terminal_ui_kit {

// Ring-buffer command history (PRD section 22). Stores submitted commands as
// owned std::string values; the oldest entry is dropped once the configured
// maximum is reached.
//
// Recall navigation:
//   * set_draft(draft) stashes the in-progress buffer before the first
//     history_previous().
//   * history_previous() walks toward older entries. The first call recalls the
//     newest entry; subsequent calls recall progressively older ones.
//   * history_next() walks toward newer entries. When the newest entry is
//     reached again it restores the stashed draft and exits navigation mode.
//   * add() records a new entry and exits navigation mode.
//   * end_navigation() exits navigation mode and clears the draft (e.g. when
//     the user edits a recalled entry directly).
class CommandHistory {
 public:
  explicit CommandHistory(std::size_t max_entries = 100);

  // Record |entry|. Empty entries and a duplicate of the most recent entry are
  // ignored. Exits navigation mode. The cursor position in the buffer after
  // recall ("newest") is the pushed entry.
  void add(std::string entry);

  void clear();
  std::size_t size() const;
  bool empty() const;

  // Entry at 0-based index relative to the oldest entry (0 == oldest).
  const std::string& at(std::size_t index) const;
  const std::deque<std::string>& entries() const;

  // Stash the in-progress draft. Does not start navigation.
  void set_draft(std::string draft);
  // True while a recall session is active.
  bool navigating() const;

  // Recall semantics described at the top of the class. Each returns false
  // when there is nothing to show (empty / not navigating).
  bool previous(std::string& out);
  bool next(std::string& out);

  // Exit navigation mode and drop the stashed draft.
  void end_navigation();

 private:
  std::deque<std::string> entries_;
  std::size_t max_entries_;
  std::string draft_;
  std::optional<std::size_t> nav_index_;
};

}  // namespace terminal_ui_kit
