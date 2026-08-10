#include "terminal_ui_kit/editor/multiline_editor.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/animation.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/box.hpp>

namespace terminal_ui_kit {
namespace {

// True for UTF-8 continuation bytes (0b10xxxxxx).
bool IsContinuationByte(unsigned char b) { return (b & 0xC0U) == 0x80U; }

// Byte index just past the code point starting at |byte| in |s|.
std::size_t NextCodePoint(const std::string& s, std::size_t byte) {
  std::size_t pos = byte + 1;
  while (pos < s.size() && IsContinuationByte(static_cast<unsigned char>(s[pos]))) {
    ++pos;
  }
  return pos;
}

// Number of code points fully contained in s[from, to). |from| must be a
// code-point boundary.
std::size_t CountCodePoints(const std::string& s, std::size_t from, std::size_t to) {
  std::size_t count = 0;
  std::size_t pos = from;
  while (pos < to) {
    pos = NextCodePoint(s, pos);
    ++count;
  }
  return count;
}

// Visible substring of |s| starting at byte |start| (a boundary), at most
// |max_code_points| code points long.
std::string TakeCodePoints(const std::string& s, std::size_t start, std::size_t max_code_points) {
  if (start >= s.size()) {
    return {};
  }
  std::size_t end = start;
  std::size_t count = 0;
  while (end < s.size() && count < max_code_points) {
    end = NextCodePoint(s, end);
    ++count;
  }
  return s.substr(start, end - start);
}

// Split |visible| at the |display_column|-th code point for caret rendering.
// The returned caret is empty when the column is at/after the end.
struct CaretSplit {
  std::string prefix;
  std::string caret;
  std::string suffix;
};

CaretSplit SplitAtCaret(const std::string& visible, std::size_t display_column) {
  std::size_t start = 0;
  std::size_t count = 0;
  while (start < visible.size() && count < display_column) {
    start = NextCodePoint(visible, start);
    ++count;
  }
  if (count < display_column || start >= visible.size()) {
    return {visible, "", ""};
  }
  const std::size_t caret_end = NextCodePoint(visible, start);
  return {visible.substr(0, start), visible.substr(start, caret_end - start),
          visible.substr(caret_end)};
}

// Wraps the rendered editor element and observes the box the terminal actually
// allocates, pushing it back into the document viewport and requesting another
// frame so the view catches up with a resize (models the VirtualList
// ObservingBoxDecorator pattern).
class ViewportObservingNode : public ftxui::Node {
 public:
  ViewportObservingNode(ftxui::Element child, EditorDocument& document)
      : Node({std::move(child)}), document_(&document) {}

  void ComputeRequirement() override {
    children_[0]->ComputeRequirement();
    requirement_ = children_[0]->requirement();
  }

  void SetBox(ftxui::Box box) override {
    const bool changed =
        document_->viewport_width() != Width(box) || document_->viewport_height() != Height(box);
    Node::SetBox(box);
    children_[0]->SetBox(box);
    if (changed) {
      document_->set_viewport_size(Width(box), Height(box));
      ftxui::animation::RequestAnimationFrame();
    }
  }

  void Render(ftxui::Screen& screen) override { children_[0]->Render(screen); }

 private:
  static std::size_t Width(const ftxui::Box& b) {
    return static_cast<std::size_t>(std::max(1, b.x_max - b.x_min + 1));
  }
  static std::size_t Height(const ftxui::Box& b) {
    return static_cast<std::size_t>(std::max(1, b.y_max - b.y_min + 1));
  }

  EditorDocument* document_;
};

ftxui::Element ObserveViewport(ftxui::Element child, EditorDocument& document) {
  return std::make_shared<ViewportObservingNode>(std::move(child), document);
}

}  // namespace

class MultilineEditorImpl : public ftxui::ComponentBase {
 public:
  explicit MultilineEditorImpl(MultilineEditorOptions options) : options_(std::move(options)) {}

  MultilineEditorOptions& options() { return options_; }
  EditorDocument& document() { return document_; }

  void set_text(std::string text) {
    document_.set_text(std::move(text));
    end_history();
  }

  bool Focusable() const override { return true; }

  bool OnEvent(ftxui::Event event) override {
    const EditorKeyBindings& kb = options_.key_bindings;

    if (event == kb.move_left) {
      document_.move_left();
      return true;
    }
    if (event == kb.move_right) {
      document_.move_right();
      return true;
    }
    if (event == kb.move_up) {
      document_.move_up();
      return true;
    }
    if (event == kb.move_down) {
      document_.move_down();
      return true;
    }
    if (event == kb.move_home) {
      document_.move_home();
      return true;
    }
    if (event == kb.move_end) {
      document_.move_end();
      return true;
    }
    if (event == kb.move_word_left) {
      document_.move_word_left();
      return true;
    }
    if (event == kb.move_word_right) {
      document_.move_word_right();
      return true;
    }
    if (event == kb.delete_backward) {
      begin_edit();
      document_.delete_backward();
      return true;
    }
    if (event == kb.delete_forward) {
      begin_edit();
      document_.delete_forward();
      return true;
    }
    if (event == kb.insert_newline) {
      begin_edit();
      document_.insert_newline();
      if (options_.on_newline) {
        options_.on_newline();
      }
      return true;
    }
    if (event == kb.history_previous) {
      return on_history_previous();
    }
    if (event == kb.history_next) {
      return on_history_next();
    }
    if (event == kb.submit) {
      end_history();
      if (options_.on_submit) {
        options_.on_submit(document_.text());
      }
      return true;
    }

    // Printable text / bracketed paste. A single Character event may carry
    // several code points (including newlines from a bracketed paste);
    // insert_text handles newline splitting.
    if (event.is_character()) {
      const std::string& text = event.character();
      if (!text.empty() && !IsControl(text)) {
        begin_edit();
        document_.insert_text(text);
        return true;
      }
    }
    return false;
  }

  ftxui::Element Render() override {
    const std::size_t top = document_.scroll_top();
    const std::size_t height = document_.viewport_height();
    const std::size_t line_count = document_.line_count();
    const std::size_t rows = std::min(height, line_count > top ? line_count - top : 0);

    ftxui::Elements lines;
    lines.reserve(rows);
    for (std::size_t r = 0; r < rows; ++r) {
      lines.push_back(render_line(top + r));
    }
    // Pad to the full viewport height so the editor occupies its box and the
    // scroll region is stable across frames.
    for (std::size_t r = rows; r < height; ++r) {
      lines.push_back(ftxui::text(""));
    }

    ftxui::Element content = ftxui::vbox(std::move(lines));
    return ObserveViewport(std::move(content), document_);
  }

 private:
  static bool IsControl(const std::string& text) {
    const unsigned char c = static_cast<unsigned char>(text[0]);
    return c < 32U || c == 127U;
  }

  ftxui::Element render_line(std::size_t line_index) {
    const std::string& raw = document_.line(line_index);
    const std::size_t start = document_.scroll_left();
    const std::size_t width = document_.viewport_width();
    std::string visible = TakeCodePoints(raw, start, width);

    if (line_index != document_.cursor().line) {
      return ftxui::text(visible);
    }

    // Cursor line: invert the character at the cursor's display column.
    const std::size_t cursor_col = document_.cursor().column;
    std::size_t display_column = 0;
    if (cursor_col >= start) {
      display_column = CountCodePoints(raw, start, cursor_col);
    }
    CaretSplit split = SplitAtCaret(visible, display_column);
    const std::string caret = split.caret.empty() ? std::string(" ") : split.caret;
    return ftxui::hbox({
        ftxui::text(split.prefix),
        ftxui::text(caret) | ftxui::inverted,
        ftxui::text(split.suffix),
    });
  }

  bool on_history_previous() {
    if (!history_available()) {
      return false;
    }
    CommandHistory* history = options_.history;
    if (!history->navigating()) {
      history->set_draft(document_.text());
    }
    std::string recalled;
    if (!history->previous(recalled)) {
      return false;
    }
    document_.set_text(std::move(recalled));
    document_.move_end();  // position the cursor at the end of the recalled line
    return true;
  }

  bool on_history_next() {
    if (!history_available()) {
      return false;
    }
    CommandHistory* history = options_.history;
    if (!history->navigating()) {
      return false;
    }
    std::string recalled;
    history->next(recalled);
    document_.set_text(std::move(recalled));
    document_.move_end();  // position the cursor at the end of the recalled line
    return true;
  }

  bool history_available() const { return options_.history_enabled && options_.history != nullptr; }

  // Typing/editing a recalled entry exits history navigation mode.
  void begin_edit() { end_history(); }

  void end_history() {
    if (options_.history != nullptr && options_.history->navigating()) {
      options_.history->end_navigation();
    }
  }

  MultilineEditorOptions options_;
  EditorDocument document_;
};

MultilineEditor::MultilineEditor(MultilineEditorOptions options)
    : impl_(std::make_shared<MultilineEditorImpl>(std::move(options))) {}

ftxui::Component MultilineEditor::component() const { return impl_; }

EditorDocument& MultilineEditor::document() const { return impl_->document(); }

void MultilineEditor::set_text(std::string text) { impl_->set_text(std::move(text)); }

}  // namespace terminal_ui_kit
