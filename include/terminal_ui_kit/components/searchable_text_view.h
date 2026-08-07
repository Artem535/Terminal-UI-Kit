#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <ftxui/component/component.hpp>

#include "terminal_ui_kit/search/search_engine.h"
#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

struct SearchableTextViewOptions {
  Theme theme = default_dark_theme();
  int tab_width = 8;
  bool follow = true;
  bool show_line_numbers = true;
  // Invoked whenever the search status changes (e.g. after applying a query).
  std::function<void(SearchStatus)> on_status_change;
};

// A reusable, searchable text view. Holds an immutable UTF-8 source document
// (see set_lines), wraps it to the available width, and renders it with all
// matches highlighted and the active match distinguished. Search is integrated
// (see the key bindings below) and the component owns all search state.
//
// Key bindings handled by the component:
//   /            open the search prompt (empty query)
//   <type>       when the prompt is open, edit the query incrementally
//   Enter        apply the query (closes the prompt, keeps results)
//   Backspace    when the prompt is open, delete the last query character
//   Esc          when open: cancel (clear the query and close); otherwise
//                falls through to the caller
//   n            next match (wraps)
//   N            previous match (wraps)
//   c            toggle case sensitivity
//   r            toggle regex mode
//
// Arrow keys / PageUp / PageDown / Home / End scroll the viewport via the
// underlying virtual list. `q` is intentionally not handled here so the host
// application can bind its own quit key.
class SearchableTextView {
 public:
  explicit SearchableTextView(SearchableTextViewOptions options = {});
  ~SearchableTextView();

  SearchableTextView(const SearchableTextView&) = delete;
  SearchableTextView& operator=(const SearchableTextView&) = delete;
  SearchableTextView(SearchableTextView&&) noexcept;
  SearchableTextView& operator=(SearchableTextView&&) noexcept;

  ftxui::Component component() const;

  // Replaces the source document. The supplied lines are copied once and then
  // never mutated; existing query results are recomputed against the new
  // document.
  void set_lines(const std::vector<std::string>& lines);

  // ---- Search prompt ----
  void open_search();
  void close_search();
  [[nodiscard]] bool search_open() const;

  // Sets the query and immediately recomputes matches.
  void set_query(std::string_view query);
  [[nodiscard]] const std::string& query() const;
  [[nodiscard]] SearchStatus status() const;

  // ---- Match navigation ----
  void next_match();
  void previous_match();
  void jump_to_first_match();
  [[nodiscard]] std::size_t match_count() const;
  [[nodiscard]] std::optional<std::size_t> current_match_index() const;

  // ---- Options ----
  [[nodiscard]] bool case_sensitive() const;
  void set_case_sensitive(bool value);
  void toggle_case_sensitive();
  [[nodiscard]] bool use_regex() const;
  void set_use_regex(bool value);
  void toggle_regex();

  // Scrolls the viewport so the current match (if any) is visible.
  void scroll_to_current_match();

 private:
  std::shared_ptr<class SearchableTextViewImpl> impl_;
};

}  // namespace terminal_ui_kit
