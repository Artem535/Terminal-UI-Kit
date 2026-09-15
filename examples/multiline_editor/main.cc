// MultilineEditor example.
//
// Demonstrates the public MultilineEditor component backed by an
// EditorDocument and a CommandHistory. The UI shows a status bar with the
// current line/column, total line count, viewport position, history status and
// the last submitted value, plus a help footer listing the active key
// bindings.
//
// Controls (default bindings, all configurable via EditorKeyBindings):
//   Arrows / Home / End      move the cursor
//   Ctrl+Left / Ctrl+Right   word navigation
//   Ctrl+Up / Ctrl+Down      history previous / next (recall mode)
//   Backspace / Delete       delete backward / forward
//   Enter                    insert a newline
//   Ctrl+Enter               Submit (prints via on_submit, adds to history)
//   Esc                      exit the application
//
// The buffer is read back after a terminal resize by the view watching its own
// box: the same EditorDocument drives the FTXUI rendering.

#include <memory>
#include <string>
#include <utility>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/editor/command_history.h"
#include "terminal_ui_kit/editor/editor_document.h"
#include "terminal_ui_kit/editor/multiline_editor.h"

using terminal_ui_kit::CommandHistory;
using terminal_ui_kit::MultilineEditor;
using terminal_ui_kit::MultilineEditorOptions;

int main() {
  CommandHistory history;
  history.add("list --all");
  history.add("show status");

  std::string last_submitted = "(none)";

  MultilineEditorOptions options;
  options.history = &history;
  options.on_submit = [&](std::string value) {
    // Don't record trivial/empty submissions.
    if (!value.empty()) {
      history.add(value);
    }
    last_submitted = value.empty() ? "(empty)" : std::move(value);
  };
  options.on_newline = [] {};

  auto editor = std::make_shared<MultilineEditor>(options);
  // Deterministic sample data so the first render is not blank.
  editor->set_text("example\nmultiline\neditor");

  auto editor_component = editor->component();

  ftxui::ScreenInteractive screen = ftxui::ScreenInteractive::TerminalOutput();

  // Live status bar rendered from the editor's document/history each frame.
  auto status_bar = [&] {
    const terminal_ui_kit::EditorDocument& doc = editor->document();
    const auto cursor = doc.cursor();
    std::string history_status = history.navigating() ? "recall active" : "off";
    history_status +=
        " (" + std::to_string(history.size()) + (history.size() == 1 ? " entry)" : " entries)");

    ftxui::Elements status;
    auto sep = ftxui::text("  |  ") | ftxui::color(ftxui::Color::GrayDark);
    status.push_back(ftxui::text("Line " + std::to_string(cursor.line + 1) + " Col " +
                                 std::to_string(cursor.column + 1)) |
                     ftxui::color(ftxui::Color::Green));
    status.push_back(sep);
    status.push_back(ftxui::text("Lines " + std::to_string(doc.line_count())) |
                     ftxui::color(ftxui::Color::Cyan));
    status.push_back(sep);
    status.push_back(ftxui::text("Viewpos top " + std::to_string(doc.scroll_top()) + " " +
                                 std::to_string(doc.viewport_width()) + "x" +
                                 std::to_string(doc.viewport_height())) |
                     ftxui::color(ftxui::Color::Yellow));
    status.push_back(sep);
    status.push_back(ftxui::text("History " + history_status) |
                     ftxui::color(ftxui::Color::Magenta));
    status.push_back(sep);
    status.push_back(ftxui::text("Last submit: " + last_submitted) |
                     ftxui::color(ftxui::Color::Blue));
    return ftxui::hbox(status) | ftxui::border;
  };

  auto footer =
      ftxui::vbox({
          ftxui::text("Arrows/Home/End move | Ctrl+Left/Right word | Ctrl+Up/Down history"),
          ftxui::text("Backspace/Delete edit | Enter newline | Ctrl+Enter submit | Esc exit"),
      }) |
      ftxui::color(ftxui::Color::GrayLight);

  auto editor_view = ftxui::Renderer(
      editor_component, [&] { return editor_component->Render() | ftxui::border | ftxui::flex; });

  auto component = ftxui::Container::Vertical({
      ftxui::Renderer(status_bar),
      editor_view,
      ftxui::Renderer([&] { return footer; }),
  });

  // Exit on Esc.
  component = ftxui::CatchEvent(std::move(component), [&](ftxui::Event event) {
    if (event == ftxui::Event::Escape) {
      screen.Exit();
      return true;
    }
    return false;
  });

  screen.Loop(component);
  return 0;
}
