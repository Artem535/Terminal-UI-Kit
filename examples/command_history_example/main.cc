// Example: CommandHistory
//
// Interactive demo of the bounded, navigable CommandHistory model for input
// and editor components. Everything here uses only the public API from
// <terminal_ui_kit/editor/command_history.h>: adding commands, Up/Down
// navigation, substring and prefix search, size/capacity reporting, clearing,
// capacity eviction and sensitive-command persistence filtering.
//
// Controls:
//   Enter        add the typed command to history
//   Up / Down    navigate previous / next history entries
//   c            clear history
//   t            toggle sensitive mode (blocks "secret"/"password" from
//                persistence while keeping them in memory)
//   q            quit
//
// The demo attaches a counting store so the "Persisted" counter visibly
// stops increasing for sensitive commands.

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/key_hint_bar.h"
#include "terminal_ui_kit/editor/command_history.h"
#include "terminal_ui_kit/theme/theme.h"

namespace {

using terminal_ui_kit::editor::CommandHistory;
using terminal_ui_kit::editor::CommandHistoryStore;
using terminal_ui_kit::editor::SensitiveCommandPolicy;

// A demo store that only counts how many commands reached the persistence
// layer, so the UI can show sensitivity filtering and eviction at work. Real
// stores would write to disk or a backing service.
class CountingStore : public CommandHistoryStore {
 public:
  void Persist(const std::string&) override { ++count; }
  int count = 0;
};

struct State {
  CommandHistory history{8};  // Small capacity so eviction is easy to see.
  std::unique_ptr<CountingStore> store = std::make_unique<CountingStore>();
  CountingStore* store_view = store.get();
  std::string command_text;
  std::string substring_query;
  std::string prefix_query;
  bool sensitive_mode = false;
  std::string status = "Type a command and press Enter. Press q to quit.";
};

// Rebuilds the persistence policy from the current sensitive_mode flag.
void rebuild_policy(State& s) {
  if (s.sensitive_mode) {
    s.history.SetPersistencePolicy(
        std::make_unique<SensitiveCommandPolicy>(std::vector<std::string>{"secret", "password"}));
  } else {
    s.history.SetPersistencePolicy(nullptr);  // Default: persist everything.
  }
}

}  // namespace

int main() {
  using namespace ftxui;
  using terminal_ui_kit::editor::SensitiveCommandPolicy;

  const terminal_ui_kit::Theme theme = terminal_ui_kit::default_dark_theme();
  State state;
  state.history.SetPersistentStore(std::move(state.store));

  auto add_current = [&state] {
    const std::size_t before = state.history.Size();
    state.history.Add(state.command_text);
    const std::size_t after = state.history.Size();
    if (after > before) {
      state.status = "Added \"" + state.command_text + "\" (size " + std::to_string(after) + "/" +
                     std::to_string(state.history.Capacity()) + ")";
    } else {
      state.status = "Ignored (blank, whitespace-only or consecutive duplicate)";
    }
    state.command_text.clear();
  };

  InputOption command_option;
  command_option.placeholder = "type a command, Enter to add";
  command_option.on_enter = add_current;
  auto command_input = Input(&state.command_text, command_option);

  InputOption substring_option;
  substring_option.placeholder = "substring search query";
  auto substring_input = Input(&state.substring_query, substring_option);

  InputOption prefix_option;
  prefix_option.placeholder = "prefix search query";
  auto prefix_input = Input(&state.prefix_query, prefix_option);

  Components children = {command_input, substring_input, prefix_input};
  auto container = Container::Vertical(std::move(children));

  auto render = [&state, &command_input, &substring_input, &prefix_input, &theme] {
    const std::vector<std::string> substring_results = state.history.Search(state.substring_query);
    const std::vector<std::string> prefix_results = state.history.SearchPrefix(state.prefix_query);

    Elements substring_lines;
    for (const std::string& c : substring_results) substring_lines.push_back(text("  " + c));
    if (substring_lines.empty()) substring_lines.push_back(text("  (none)") | dim);

    Elements prefix_lines;
    for (const std::string& c : prefix_results) prefix_lines.push_back(text("  " + c));
    if (prefix_lines.empty()) prefix_lines.push_back(text("  (none)") | dim);

    const std::string size_line = "History: " + std::to_string(state.history.Size()) + " / " +
                                  std::to_string(state.history.Capacity()) +
                                  "  (exceeding capacity evicts the oldest command)";
    const std::string persisted_line = "Persisted: " + std::to_string(state.store_view->count);
    const std::string sensitive_line =
        std::string("Sensitive mode: ") + (state.sensitive_mode ? "ON" : "OFF") +
        (state.sensitive_mode ? "  (secret / password stay in memory, not persisted)"
                              : "  (every command is persisted)");

    Elements body;
    body.push_back(text("Terminal UI Kit — CommandHistory") | bold);
    body.push_back(separator());
    body.push_back(text(size_line) | dim);
    body.push_back(text(persisted_line) | dim);
    body.push_back(text(sensitive_line) | (state.sensitive_mode ? color(Color::Yellow) : dim));
    body.push_back(separator());
    body.push_back(text("Command:") | bold);
    body.push_back(command_input->Render());
    body.push_back(hbox({text("         "), text(state.status) | dim}));
    body.push_back(separator());

    Elements left;
    left.push_back(text("Substring search:") | bold);
    left.push_back(substring_input->Render());
    left.push_back(separator());
    for (auto& e : substring_lines) left.push_back(std::move(e));
    Element left_col = vbox(std::move(left)) | flex;

    Elements right;
    right.push_back(text("Prefix search:") | bold);
    right.push_back(prefix_input->Render());
    right.push_back(separator());
    for (auto& e : prefix_lines) right.push_back(std::move(e));
    Element right_col = vbox(std::move(right)) | flex;

    body.push_back(hbox({std::move(left_col), separator(), std::move(right_col)}));
    body.push_back(separator());
    body.push_back(text("Controls") | bold);
    body.push_back(KeyHintBar({{"enter", "add command"},
                               {"up/down", "navigate history"},
                               {"c", "clear history"},
                               {"t", "toggle sensitive"},
                               {"q", "quit"}},
                              theme));

    return vbox(std::move(body)) | border | flex;
  };

  auto root = Renderer(container, render);

  auto screen = ScreenInteractive::Fullscreen();
  root |= CatchEvent([&state, &screen](Event event) {
    if (event == Event::Character('q')) {
      screen.ExitLoopClosure()();
      return true;
    }
    if (event == Event::Character('c')) {
      state.history.Clear();
      state.status = "History cleared";
      return true;
    }
    if (event == Event::Character('t')) {
      state.sensitive_mode = !state.sensitive_mode;
      rebuild_policy(state);
      state.status = state.sensitive_mode ? "Sensitive mode ON" : "Sensitive mode OFF";
      return true;
    }
    if (event == Event::ArrowUp) {
      if (const auto command = state.history.Previous(); command.has_value()) {
        state.command_text = *command;
        state.status = "Navigated Up";
      } else {
        state.status = "No older command";
      }
      return true;
    }
    if (event == Event::ArrowDown) {
      if (const auto command = state.history.Next(); command.has_value()) {
        state.command_text = *command;
        state.status = "Navigated Down";
      } else {
        state.command_text.clear();
        state.status = "At newest entry (editing a fresh command)";
      }
      return true;
    }
    return false;
  });

  screen.Loop(root);
  return 0;
}
