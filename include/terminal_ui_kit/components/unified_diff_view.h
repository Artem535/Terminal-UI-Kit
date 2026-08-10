#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <ftxui/component/component.hpp>

#include "terminal_ui_kit/diff/diff_model.h"
#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

// Options for UnifiedDiffView (PRD section 29).
struct UnifiedDiffViewOptions {
  // Theme used for diff styles. When `color` is false all roles are passed
  // through without_color() so meaning is carried by the +/- markers and the
  // gutter rather than by color.
  Theme theme = default_dark_theme();
  // If false, colors are stripped (no-color fallback); bold/dim/underline
  // attributes from the theme remain.
  bool color = true;
  // Whether to render the old/new line-number gutter. Always rendered when
  // true except when the viewport is too narrow (see narrow-terminal
  // fallback below).
  bool show_line_numbers = true;
  // Minimum content width (in cells) to keep before falling back to the
  // narrow-terminal mode. When the viewport cannot fit the gutter plus this
  // many content columns, the gutter is hidden and the whole width is given
  // to content. Line numbers reappear automatically once the terminal is
  // resized wide enough.
  int min_content_width = 20;
  // Called with the original source text of the selected line when the user
  // requests a copy (`y`). The text is the marker-stripped diff content (the
  // actual source line), concatenated across the line's styled spans. For
  // file/hunk header rows the header text is passed.
  std::function<void(std::string)> on_copy;
};

// A reusable, virtualized, interactive unified-diff viewer (PRD section 29).
// It displays a parsed diff model (std::vector<diff::DiffFile>, typically from
// UnifiedDiffParser) as a scrolling, virtualized list of diff rows.
//
// The view owns the model: the file list is moved/copied into the view and all
// cross references are stored as indices into that retained model, so the view
// never retains string_view/span/pointer references into caller-owned buffers.
//
// Row model
// ---------
// Every diff row is one terminal cell tall: file headers, hunk headers,
// notices (new/deleted/binary/empty file) and individual context/addition/
// deletion lines each occupy a single visual row. Long lines are truncated
// (never wrapped) with a single trailing `…`, which keeps row height at 1 and
// makes the flattened layout a fixed-height row list that can be virtualized
// without per-frame layout rebuilds.
//
// Navigation
// ----------
//   j/k or Up/Down — scroll/select a row
//   ] / [          — next/previous file
//   n / N          — next/previous hunk
//   Enter          — collapse/expand the current file
//   /              — begin a search (see set_search)
//   y              — invoke on_copy with the selected line's source text
//   Mouse wheel, PageUp/Down, Home/End
//
// Collapsed files keep their collapsed state during all navigation; toggling a
// file rebuilds the row list but preserves the selected file and, where still
// valid, the selected row.
class UnifiedDiffView {
 public:
  // Takes ownership of `files` (moved/copied into retained storage).
  explicit UnifiedDiffView(std::vector<diff::DiffFile> files, UnifiedDiffViewOptions options = {});

  ~UnifiedDiffView();
  UnifiedDiffView(const UnifiedDiffView&) = delete;
  UnifiedDiffView& operator=(const UnifiedDiffView&) = delete;

  ftxui::Component component() const;

  // --- File / hunk navigation -------------------------------------------
  void next_file();
  void prev_file();
  void next_hunk();
  void prev_hunk();

  // --- Collapse / expand --------------------------------------------------
  void toggle_collapse_current_file();
  void collapse_file(std::size_t file_index);
  void expand_file(std::size_t file_index);
  [[nodiscard]] bool is_collapsed(std::size_t file_index) const;

  // --- Search ---------------------------------------------------------------
  // Sets the search query and immediately finds the first match. An empty
  // query clears the search. Searching scans the retained model once and
  // stores the matches; it does not rescan on every frame.
  void set_search(const std::string& query);
  void jump_to_next_match();
  void jump_to_prev_match();
  [[nodiscard]] bool has_matches() const;
  [[nodiscard]] std::size_t match_count() const;
  [[nodiscard]] std::optional<std::size_t> current_match() const;

  // --- Selection / copy -----------------------------------------------------
  // Selects a specific row in the flattened layout (0-based). Rows are laid
  // out in model order; use status()/row_count() to map to visible content.
  // Returns true if the selection changed.
  bool select_row(std::size_t row_index);
  void scroll_to_row(std::size_t row_index);
  // Invokes on_copy with the selected row's source text.
  bool copy_selection();
  [[nodiscard]] bool has_selection() const;

  // --- Status ----------------------------------------------------------------
  struct Status {
    std::size_t file_index = 0;
    std::size_t file_count = 0;
    std::size_t hunk_index = 0;
    std::size_t hunk_count = 0;
    std::size_t selected_row = 0;
    std::size_t row_count = 0;
    std::size_t first_visible = 0;
    std::size_t last_visible = 0;
  };
  [[nodiscard]] Status status() const;
  [[nodiscard]] std::size_t row_count() const;
  [[nodiscard]] bool is_empty() const;

 private:
  class Impl;
  std::shared_ptr<Impl> impl_;
};

}  // namespace terminal_ui_kit
