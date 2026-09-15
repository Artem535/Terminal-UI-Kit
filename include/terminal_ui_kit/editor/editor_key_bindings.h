#pragma once

#include <ftxui/component/event.hpp>

namespace terminal_ui_kit {

// Configurable mapping from terminal events to editor commands (PRD section
// 21.2, "configurable key bindings").
//
// Each field names an ftxui::Event that triggers the corresponding command.
// All fields may be reassigned by the caller to match a custom layout; a
// default mapping suited to a typical terminal is provided by the
// default-initialized struct.
struct EditorKeyBindings {
  ftxui::Event move_left = ftxui::Event::ArrowLeft;
  ftxui::Event move_right = ftxui::Event::ArrowRight;
  ftxui::Event move_up = ftxui::Event::ArrowUp;
  ftxui::Event move_down = ftxui::Event::ArrowDown;
  ftxui::Event move_home = ftxui::Event::Home;
  ftxui::Event move_end = ftxui::Event::End;
  // Word navigation: Ctrl+Left / Ctrl+Right.
  ftxui::Event move_word_left = ftxui::Event::ArrowLeftCtrl;
  ftxui::Event move_word_right = ftxui::Event::ArrowRightCtrl;
  // History navigation: Ctrl+Up / Ctrl+Down.
  ftxui::Event history_previous = ftxui::Event::ArrowUpCtrl;
  ftxui::Event history_next = ftxui::Event::ArrowDownCtrl;
  ftxui::Event delete_backward = ftxui::Event::Backspace;
  ftxui::Event delete_forward = ftxui::Event::Delete;
  ftxui::Event insert_newline = ftxui::Event::Return;
  // Submit: Ctrl+Enter (kitty keyboard protocol CSI-u sequence). Many
  // terminals do not distinguish Ctrl+Enter from Enter; rebind this to a key
  // your terminal emits (e.g. Alt+Enter = \x1b[13;3u) if needed.
  ftxui::Event submit = ftxui::Event::Special("\x1B[13;5u");
};

}  // namespace terminal_ui_kit
