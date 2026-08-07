#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace terminal_ui_kit {

// Options controlling how a search query is matched against a document.
//
// Matching is byte-oriented: SearchEngine operates on the raw UTF-8 bytes of
// each source line (see SearchEngine::search).
struct SearchOptions {
  // When false (default), ASCII letters are matched case-insensitively
  // (both in literal and regex mode, via std::regex_constants::icase).
  // Non-ASCII code points are never case-folded: they must match in the
  // exact case present in the document.
  bool case_sensitive = false;
  // When true, the query is interpreted as an ECMAScript regular expression
  // (std::regex) matched against each line's bytes.
  bool use_regex = false;
};

// A single search hit. Start/end byte offsets are relative to the UTF-8 bytes
// of the source line `line` (line is a zero-based logical-line index; the
// trailing newline is not part of the line). `end_byte` is exclusive.
//
// For a valid UTF-8 query matched in valid UTF-8 text the offsets always land
// on UTF-8 code-point boundaries, so they never split a multi-byte sequence.
// These are source byte offsets -- distinct from terminal-cell columns.
struct TextMatch {
  std::size_t line = 0;
  std::size_t start_byte = 0;  // inclusive
  std::size_t end_byte = 0;    // exclusive

  friend bool operator==(const TextMatch&, const TextMatch&) = default;
};

// Outcome of running a query against a document.
enum class SearchStatus {
  kEmptyQuery,    // no query -> nothing searched, no matches computed
  kMatches,       // query applied and produced at least one match
  kNoResults,     // query applied but produced zero (non-empty) matches
  kInvalidRegex,  // regex mode but the pattern could not be compiled
};

// Pure-data search engine. It does not own the document and never mutates it;
// search reads every line without copying it. It is independent of FTXUI so
// it can be unit-tested and reused without a terminal backend.
class SearchEngine {
 public:
  // Searches `lines` for `query` and appends all matches to `out_matches`
  // (replacing its prior contents), ordered by (line, start_byte). Returns the
  // resulting status. Single matches never span lines: a pattern containing a
  // newline can still match at most within one line. Zero-length matches
  // (start_byte == end_byte) are not reported.
  static SearchStatus search(std::span<const std::string_view> lines, std::string_view query,
                             const SearchOptions& options, std::vector<TextMatch>& out_matches);
};

// Tracks an ordered match list plus an active-match cursor, with wrap-around
// navigation. Owns the matches (safe owning storage) so callers never retain
// borrowed views into external state.
class MatchNavigator {
 public:
  MatchNavigator() = default;

  // Replaces the match list with `matches`. When the new list is non-empty the
  // cursor is preserved when possible: if the previously active match still
  // exists it is re-selected, otherwise the previous cursor index is clamped
  // into range; with no prior cursor the first match becomes current. When the
  // new list is empty the cursor is cleared.
  void set_matches(std::vector<TextMatch> matches);

  // Clears all matches and the cursor.
  void clear();

  [[nodiscard]] bool has_matches() const { return !matches_.empty(); }
  [[nodiscard]] std::size_t count() const { return matches_.size(); }
  [[nodiscard]] const std::vector<TextMatch>& matches() const { return matches_; }

  // Sets the current match index; clamped into range. No-op when empty.
  void set_current(std::size_t index);

  [[nodiscard]] std::optional<std::size_t> current_index() const { return current_; }
  // The active match, or std::nullopt when there are no matches.
  [[nodiscard]] std::optional<TextMatch> current() const;

  // Advance to the next/previous match, wrapping around the ends.
  void next();
  void previous();

  // Move the cursor to the first match.
  void jump_to_first();

 private:
  std::vector<TextMatch> matches_;
  std::optional<std::size_t> current_;
};

}  // namespace terminal_ui_kit
