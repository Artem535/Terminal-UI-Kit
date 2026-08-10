#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "terminal_ui_kit/components/status.h"
#include "terminal_ui_kit/document/log_model.h"

namespace terminal_ui_kit {

// A block of plain text (PRD section 63, transcript primitives).
struct TextBlock {
  std::string text;
};

// A block whose content is Markdown source. Rendering is intentionally kept
// plain-text here so the transcript works without the optional Markdown
// module; applications may substitute a richer renderer via the view.
struct MarkdownBlock {
  std::string markdown;
};

// A block of source code with an optional language tag.
struct CodeBlock {
  std::string code;
  std::string language;
};

// A single structured log line, reusing the shared severity enum.
struct LogBlock {
  LogSeverity severity = LogSeverity::kInfo;
  std::string message;
};

// A raw unified-diff text block, rendered with +/- coloring.
struct DiffBlock {
  std::string diff;
};

// A semantic status line (running, success, error, ...) with a label.
struct StatusBlock {
  Status status = Status::kIdle;
  std::string text;
};

// An application-defined block carrying a label and pre-rendered content.
struct CustomBlock {
  std::string label;
  std::string content;
};

// The heterogeneous set of block kinds a transcript can hold.
using TranscriptItem = std::variant<TextBlock, MarkdownBlock, CodeBlock, LogBlock, DiffBlock,
                                    StatusBlock, CustomBlock>;

// Owns the transcript data: an ordered list of immutable completed blocks plus
// at most one mutable streaming tail block. Provides derived services (plain
// text extraction, search index, bookmarks) used by the view and testable
// without a terminal.
//
// Thread-safety: NOT thread-safe. Mutating methods (append/append_tail/find/
// clear/...) and readers must be called from a single thread (e.g. the UI
// thread). A background producer that wants to stream into the tail must marshal
// the update onto the same thread (the example does this via posted events).
class TranscriptModel {
 public:
  TranscriptModel() = default;

  // Appends a completed (immutable) block and returns its index. Any active
  // mutable tail is finalized first so a completed block is always appended
  // at the end and the tail is always the last block.
  std::size_t append(TranscriptItem item);

  // Begins a new mutable streaming tail block, appending it as the last block.
  // Returns the tail's index. Any previously active tail is finalized first.
  std::size_t begin_tail(TranscriptItem initial);

  // Whether a mutable streaming tail currently exists.
  [[nodiscard]] bool has_tail() const;
  [[nodiscard]] std::optional<std::size_t> tail_index() const;

  // Streams a chunk into the existing tail block without creating a new
  // transcript entry. No-op when there is no active tail.
  void append_tail(std::string_view chunk);

  // Replaces the tail's content entirely (carriage-return style update).
  void replace_tail(std::string_view content);

  // Promotes the tail to an ordinary completed block.
  void finalize_tail();

  // Total number of blocks, the mutable tail counting as one block.
  [[nodiscard]] std::size_t block_count() const;

  // The block at `index`. Throws std::out_of_range on an invalid index.
  const TranscriptItem& block_at(std::size_t index) const;

  // The block's full plain text (used for copy and search indexing).
  [[nodiscard]] std::string block_plain_text(std::size_t index) const;

  // The block's display lines, split on '\n'. The tail returns its current
  // (possibly large) set of lines; the view clips rendering to a fixed height.
  [[nodiscard]] std::vector<std::string> block_lines(std::size_t index) const;

  // The tail's display lines clipped to at most `max_lines` single lines,
  // taken from the END of the tail content. Unlike block_lines() this does not
  // split the whole (possibly very large) tail, so it stays O(max_lines + last
  // line) per call — the hot path used to render the fixed-height streaming
  // tail. Returns an empty vector when there is no active tail.
  [[nodiscard]] std::vector<std::string> tail_visible_lines(std::size_t max_lines) const;

  // Monotonic revision bumped on every data mutation. Used to detect changes
  // for follow and for search-index caching.
  [[nodiscard]] std::uint64_t revision() const { return revision_; }

  // Removes every block (including the tail) and resets derived state.
  void clear();

  // --- Search -------------------------------------------------------------
  // Sets the active query and returns the first matching block index, or
  // std::nullopt. The hit list is cached and only rebuilt when the query or
  // the data revision changes.
  std::optional<std::size_t> find(const std::string& query);

  // First matching block index after `after` (strictly greater), or nullopt.
  [[nodiscard]] std::optional<std::size_t> next_match(std::size_t after) const;

  // Last matching block index strictly before `before`, or nullopt.
  [[nodiscard]] std::optional<std::size_t> previous_match(std::size_t before) const;

  // The cached hit list (ascending), empty when no query is active.
  [[nodiscard]] const std::vector<std::size_t>& matches() const { return matches_; }
  [[nodiscard]] std::size_t match_count() const { return matches_.size(); }

  // --- Bookmarks ----------------------------------------------------------
  void toggle_bookmark(std::size_t index);
  [[nodiscard]] bool is_bookmarked(std::size_t index) const;
  [[nodiscard]] std::vector<std::size_t> bookmarks() const;

 private:
  void rebuild_search_index();

  std::vector<TranscriptItem> blocks_;
  std::optional<std::size_t> tail_;
  std::string search_query_;
  std::vector<std::size_t> matches_;
  std::uint64_t search_revision_ = 0;
  std::set<std::size_t> bookmarks_;
  std::uint64_t revision_ = 0;
};

}  // namespace terminal_ui_kit
