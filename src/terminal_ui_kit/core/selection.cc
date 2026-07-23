#include "terminal_ui_kit/core/selection.h"

#include "terminal_ui_kit/document/streaming_document.h"

namespace terminal_ui_kit {

void SelectionManager::start(TextPosition pos) {
  anchor_ = pos;
  active_ = pos;
  selecting_ = true;
}

void SelectionManager::extend_to(TextPosition pos) {
  active_ = pos;
}

void SelectionManager::clear() {
  anchor_.reset();
  active_.reset();
  selecting_ = false;
}

bool SelectionManager::has_selection() const {
  return selecting_ && anchor_.has_value() && active_.has_value() &&
         anchor_ != active_;
}

TextRange SelectionManager::range() const {
  TextRange r;
  if (anchor_ && active_) {
    if (*anchor_ <= *active_) {
      r.start = *anchor_;
      r.end = *active_;
    } else {
      r.start = *active_;
      r.end = *anchor_;
    }
  }
  return r;
}

bool SelectionManager::is_selecting() const {
  return selecting_;
}

std::string SelectionManager::selected_text(
    const StreamingDocument& doc) const {
  if (!has_selection()) {
    return {};
  }

  auto r = range();
  std::string result;

  if (r.start.line == r.end.line) {
    auto line = doc.line_at(r.start.line);
    result = std::string(line.substr(r.start.column, r.end.column - r.start.column));
  } else {
    // First line: from start.column to end of line
    {
      auto line = doc.line_at(r.start.line);
      result = std::string(line.substr(r.start.column));
    }

    // Middle lines: entire lines
    for (auto l = r.start.line + 1; l < r.end.line; ++l) {
      result += '\n';
      result += doc.line_at(l);
    }

    // Last line: from 0 to end.column
    result += '\n';
    auto line = doc.line_at(r.end.line);
    result += std::string(line.substr(0, r.end.column));
  }

  return result;
}

}  // namespace terminal_ui_kit
