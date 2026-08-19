#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <ftxui/component/component.hpp>

#include "terminal_ui_kit/diff/diff_model.h"
#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

struct UnifiedDiffViewOptions {
  // Semantic palette used for file headers, hunk headers, additions,
  // deletions, and context. The view never re-parses the model and never
  // generates diffs; it only styles already-parsed diff lines.
  Theme theme = default_dark_theme();
  // When false, color is stripped from every style role (via
  // `Theme::without_color`) while text, markers, and line numbers remain
  // visible, providing a no-color fallback for monochrome terminals.
  bool enable_color = true;
  // Invoked when the user copies (y) a selected diff line. Receives the
  // line's original source text (the retained content, without the diff
  // `+`/`-`/space marker or line numbers). Non-line rows call back with an
  // empty string.
  std::function<void(std::string)> on_copy;
};

class UnifiedDiffViewImpl;

// A virtualized, viewport-based unified-diff view (PRD section 29) built on
// top of the canonical unified-diff model produced by `UnifiedDiffParser`.
//
// The view renders `std::vector<diff::DiffFile>` rows through `VirtualList`,
// so only the visible rows are materialized per frame; a 100,000-line diff is
// scrolled without building the whole document each frame. A flat row index is
// rebuilt lazily — only when the model changes (`SetFiles`) or a file's
// collapse state toggles — and is never rebuilt on resize, keeping the
// selected file/hunk/row logically stable across a terminal resize.
//
// Long lines are truncated to the content width (viewport minus gutter and
// marker) with a `…` continuation marker rather than wrapped, so every row is
// exactly one terminal line and row heights never depend on the terminal
// width.
class UnifiedDiffView {
 public:
  explicit UnifiedDiffView(UnifiedDiffViewOptions options);
  ~UnifiedDiffView() = default;

  // The interactive component. Wire this into an FTXUI screen/container.
  ftxui::Component component() const;

  // Replaces the displayed diff. The model is copied into and retained by the
  // view (no pointers, spans, or string_views are retained beyond the caller's
  // lifetime); the caller may destroy its own copy immediately after.
  void SetFiles(std::vector<diff::DiffFile> files);
  const std::vector<diff::DiffFile>& files() const;

  // Model/selection introspection (for status lines and tests).
  std::size_t file_count() const;
  // 0-based index of the file containing the current selection.
  std::size_t selected_file() const;
  // 0-based index of the hunk (within the selected file) containing the
  // current selection.
  std::size_t selected_hunk() const;
  // {first_visible_row_index, visible_row_count} of the most recent render.
  std::pair<std::size_t, std::size_t> visible_range() const;
  // Number of times the internal flat row layout has been (re)built since
  // construction. Stays constant across renders and resizes; increments only
  // when the model changes or a file's collapse state toggles. Useful for
  // verifying the "do not rebuild the layout every frame" performance
  // guarantee.
  std::size_t layout_build_count() const;
  // A ready-to-display status line, e.g.
  //   File 2/5 · Hunk 3/8 · Visible rows 120–160
  std::string status_line() const;

  // --- Navigation ---
  // Move selection to the next/previous hunk within the selected file
  // (clamped to the file's first/last hunk).
  void next_hunk();
  void previous_hunk();
  // Move selection to the next/previous file (clamped to first/last file).
  void next_file();
  void previous_file();
  // Collapse/expand the selected file. Collapsed state persists across
  // navigation.
  void toggle_collapse();
  bool collapsed(std::size_t file_index) const;

  // --- Search ---
  // Set (or clear, with "") the active search query. Search is a
  // case-insensitive ASCII substring match over line content (multi-byte
  // UTF-8 characters are matched byte-for-byte, so searching for non-ASCII
  // text is supported only as an exact byte sequence). While a query is
  // active, `n`/`N` navigate search results instead of hunks.
  void set_search(const std::string& query);
  const std::string& search() const;
  // Number of rows matching the active query (0 when none).
  std::size_t search_result_count() const;
  // Move selection to the next/previous search result (wraps), if any.
  void next_search_result();
  void previous_search_result();

  // --- Copy ---
  // Invokes `on_copy` with the selected line's original source text. Safe to
  // call when nothing is selected or a non-line row is selected (empty text).
  void copy_selected();

 private:
  std::shared_ptr<UnifiedDiffViewImpl> impl_;
};

}  // namespace terminal_ui_kit
