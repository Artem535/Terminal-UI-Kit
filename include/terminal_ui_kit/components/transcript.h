#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>

#include <ftxui/component/component.hpp>

#include "terminal_ui_kit/components/transcript_model.h"
#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

struct TranscriptViewOptions {
  Theme theme = default_dark_theme();
  // Whether the view follows the end of the transcript (auto-scroll on data
  // change) until the user scrolls manually.
  bool follow = true;
  // Fixed display height (in rows) of the mutable streaming tail block. The
  // tail always renders this many rows (last lines, padded) so appending to
  // it never changes its measured height and never invalidates layout.
  int tail_display_height = 8;
  // Invoked with the plain text of the selected block when the copy action
  // ('y') is triggered.
  std::function<void(std::string text)> on_copy;
  // Invoked with the selected block index when details ('Enter') is triggered.
  std::function<void(std::size_t index)> on_open_details;
};

class TranscriptViewImpl;

// A virtualized, follow-end transcript of heterogeneous blocks with a
// mutable streaming tail, search, bookmarks, and copy/details actions.
class TranscriptView {
 public:
  // `model` must outlive the view.
  explicit TranscriptView(TranscriptModel* model, TranscriptViewOptions options = {});

  ftxui::Component component() const;

  [[nodiscard]] bool follow() const;
  void set_follow(bool follow);
  void scroll_to_bottom();
  void scroll_to_index(std::size_t index);

  // Search. find() sets the active query and jumps to the first match;
  // find_next()/find_previous() move through the cached hits (wrapping).
  bool find(const std::string& query);
  bool find_next();
  bool find_previous();
  [[nodiscard]] std::optional<std::size_t> current_match() const;
  // Whether the interactive search input ('/') is currently active.
  [[nodiscard]] bool search_mode() const;

 private:
  std::shared_ptr<TranscriptViewImpl> impl_;
};

}  // namespace terminal_ui_kit
