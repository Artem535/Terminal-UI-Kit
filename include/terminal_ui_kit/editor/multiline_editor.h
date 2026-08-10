#pragma once

#include <functional>
#include <memory>
#include <string>

#include <ftxui/component/component.hpp>

#include "terminal_ui_kit/editor/command_history.h"
#include "terminal_ui_kit/editor/editor_document.h"
#include "terminal_ui_kit/editor/editor_key_bindings.h"

namespace terminal_ui_kit {

// MultilineEditor is the FTXUI view/controller for an EditorDocument.
//
// It renders the document's visible region (honouring the model viewport so
// the cursor stays visible), translates keyboard events into model editing and
// navigation commands, and delivers the submitted buffer through the
// on_submit callback. The document is owned by the editor; retrieve it with
// document() to inspect or drive state directly.
//
// History integration: when |history| is set, the history_previous /
// history_next bindings recall and restore entries as documented on
// CommandHistory. Editing text while navigating exits history mode.
struct MultilineEditorOptions {
  // Invoked with a copy of the final buffer when the submit key is pressed.
  std::function<void(std::string)> on_submit;
  // Optional; invoked after a newline is inserted.
  std::function<void()> on_newline;
  EditorKeyBindings key_bindings;
  // Optional completion history used for recall. Not owned by the editor.
  CommandHistory* history = nullptr;
  bool history_enabled = true;
};

class MultilineEditorImpl;

class MultilineEditor {
 public:
  explicit MultilineEditor(MultilineEditorOptions options = {});

  // The FTXUI component used to render and drive the editor.
  ftxui::Component component() const;

  // The underlying document model (shared, mutable).
  EditorDocument& document() const;
  // Replace the whole document content; resets the cursor to the start.
  void set_text(std::string text);

 private:
  std::shared_ptr<MultilineEditorImpl> impl_;
};

}  // namespace terminal_ui_kit
