// CommandHistory interactive example.
//
// Demonstrates terminal_ui_kit::command::CommandHistory through its public API:
// adding commands, Up/Down navigation, substring and prefix search, current
// size/capacity, clearing, toggling a sensitive-command policy, and capacity
// eviction.
//
// The history model itself is pure data (no FTXUI); FTXUI is used here only to
// build the interactive shell. Persistence is demonstrated with a small
// in-memory adapter and the example's commands go through the public
// CommandHistory API (never a private command vector).
//
// Controls (also rendered in the UI):
//   Enter       — add the command line to history
//   Up / Down   — navigate history (Previous / Next)
//   Tab         — move focus between the command line and the search box
//   F1          — clear history
//   F2          — toggle sensitive mode
//   F3          — toggle search mode (substring <-> prefix)
//   Esc         — quit

#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/command/command_history.h"

using terminal_ui_kit::command::CommandHistory;
using terminal_ui_kit::command::CommandHistoryPersistence;
using terminal_ui_kit::command::NeverSensitivePolicy;
using terminal_ui_kit::command::PrefixSensitivePolicy;

namespace {

// In-memory persistence adapter for the demo. Writes to an external "store"
// vector and records every operation in the event log. Can be asked to fail so
// the "persistence errors never corrupt in-memory history" property is visible.
class DemoPersistence final : public CommandHistoryPersistence {
 public:
  bool Save(const std::string& command) override {
    if (fail_persist) {
      log->push_back("persist FAILED (ignored): " + command);
      return false;
    }
    store->push_back(command);
    log->push_back("persisted: " + command);
    return true;
  }

  bool Clear() override {
    if (fail_clear) {
      log->push_back("store clear FAILED (ignored)");
      return false;
    }
    store->clear();
    log->push_back("store cleared");
    return true;
  }

  std::vector<std::string>* store = nullptr;
  std::vector<std::string>* log = nullptr;
  bool fail_persist = false;
  bool fail_clear = false;
};

struct AppState {
  CommandHistory history{5};
  std::shared_ptr<DemoPersistence> persistence = std::make_shared<DemoPersistence>();
  bool sensitive_mode = false;
  bool prefix_search = false;

  std::string command_line;
  std::string query;

  std::vector<std::string> store;   // mock persistence store
  std::vector<std::string> events;  // event log (newest first)
  std::size_t evicted = 0;

  void Log(std::string message) {
    events.insert(events.begin(), std::move(message));
    if (events.size() > 8) events.pop_back();
  }
};

void AddCurrent(AppState& s) {
  if (s.command_line.empty()) return;

  const std::size_t before = s.history.Size();
  const bool sensitive_now = s.sensitive_mode && s.command_line.rfind("secret:", 0) == 0;
  s.history.Add(s.command_line);
  const std::size_t after = s.history.Size();

  if (after == before) {
    if (s.history.Capacity() == 0) {
      s.Log("capacity is 0 — nothing retained");
    } else {
      s.Log("ignored (blank or consecutive duplicate)");
    }
  } else {
    if (before > 0 && before == s.history.Capacity()) {
      ++s.evicted;
      s.Log("added (evicted oldest)");
    } else {
      s.Log("added");
    }
    if (sensitive_now) {
      s.Log("sensitive — kept in memory, not persisted");
    }
    s.command_line.clear();
  }
}

ftxui::Element ControlsLine() {
  return ftxui::hbox({
      ftxui::text(" Enter:add ") | ftxui::inverted,
      ftxui::text(" Up/Down:nav ") | ftxui::inverted,
      ftxui::text(" Tab:focus ") | ftxui::inverted,
      ftxui::text(" F1:clear ") | ftxui::inverted,
      ftxui::text(" F2:sensitive ") | ftxui::inverted,
      ftxui::text(" F3:search-mode ") | ftxui::inverted,
      ftxui::text(" Esc:quit ") | ftxui::inverted,
  });
}

ftxui::Element StatusLine(const AppState& s) {
  return ftxui::hbox({
      ftxui::text("size=" + std::to_string(s.history.Size())),
      ftxui::text(" capacity=" + std::to_string(s.history.Capacity())),
      ftxui::text(s.sensitive_mode ? "  [sensitive: ON]" : "  [sensitive: OFF]"),
      ftxui::text("  evicted=" + std::to_string(s.evicted)),
  });
}

ftxui::Element HistoryPanel(const AppState& s) {
  ftxui::Elements lines;
  if (s.history.Search("").empty()) {
    lines.push_back(ftxui::text("(empty)") | ftxui::dim);
  } else {
    const std::vector<std::string> all = s.history.Search("");
    for (std::size_t i = 0; i < all.size(); ++i) {
      lines.push_back(ftxui::text("  " + std::to_string(i) + "  " + all[i]));
    }
  }
  return ftxui::window(ftxui::text("History (memory)"), ftxui::vbox(std::move(lines)));
}

ftxui::Element SearchPanel(const AppState& s) {
  std::vector<std::string> results =
      s.prefix_search ? s.history.SearchPrefix(s.query) : s.history.Search(s.query);
  ftxui::Elements lines;
  for (const std::string& r : results) lines.push_back(ftxui::text("  " + r));
  const std::string mode = s.prefix_search ? "prefix" : "substring";
  return ftxui::window(ftxui::text("Search [" + mode + "] of \"" + s.query + "\"  (" +
                                   std::to_string(results.size()) + ")"),
                       results.empty() ? ftxui::vbox({ftxui::text("(no matches)") | ftxui::dim})
                                       : ftxui::vbox(std::move(lines)));
}

ftxui::Element PersistencePanel(const AppState& s) {
  ftxui::Elements lines;
  for (const std::string& entry : s.store) lines.push_back(ftxui::text("  " + entry));
  return ftxui::window(ftxui::text("Persistence store (" + std::to_string(s.store.size()) + ")"),
                       s.store.empty()
                           ? ftxui::vbox({ftxui::text("(nothing persisted yet)") | ftxui::dim})
                           : ftxui::vbox(std::move(lines)));
}

ftxui::Element EventLogPanel(const AppState& s) {
  ftxui::Elements lines;
  for (const std::string& e : s.events) lines.push_back(ftxui::text("  " + e));
  return ftxui::window(ftxui::text("Event log"), ftxui::vbox(std::move(lines)));
}

}  // namespace

int main() {
  ftxui::ScreenInteractive screen = ftxui::ScreenInteractive::Fullscreen();

  AppState state;
  state.persistence->store = &state.store;
  state.persistence->log = &state.events;

  // Deterministic sample data (capacity 5): several commands evict the oldest.
  for (const char* seed :
       {"ls -la", "cd projects", "git status", "git diff", "cmake --build --preset debug",
        "ctest --preset debug", "git push origin main"}) {
    state.command_line = seed;
    AddCurrent(state);
  }
  state.command_line.clear();

  auto command_input = ftxui::Input(&state.command_line, "type a command, Enter to add");
  auto search_input = ftxui::Input(&state.query, "search...");

  // Intercept Enter and arrow keys on the command line for history behaviour.
  auto command_with_nav = ftxui::CatchEvent(command_input, [&](const ftxui::Event& e) {
    if (e == ftxui::Event::ArrowUp) {
      if (const auto prev = state.history.Previous()) state.command_line = *prev;
      return true;
    }
    if (e == ftxui::Event::ArrowDown) {
      if (const auto next = state.history.Next()) state.command_line = *next;
      return true;
    }
    if (e == ftxui::Event::Return) {
      AddCurrent(state);
      return true;
    }
    return false;
  });

  auto container = ftxui::Container::Vertical({command_with_nav, search_input});

  auto renderer = ftxui::Renderer(container, [&] {
    const bool cmd_active = command_input->Focused();
    const bool search_active = search_input->Focused();
    return ftxui::vbox({
               ControlsLine(),
               ftxui::separator(),
               ftxui::hbox({
                   ftxui::text(cmd_active ? "> " : "  "),
                   command_input->Render() | ftxui::flex,
               }) | ftxui::border,
               ftxui::hbox({
                   ftxui::text(search_active ? "> " : "  "),
                   search_input->Render() | ftxui::flex,
               }) | ftxui::border,
               StatusLine(state),
               ftxui::separator(),
               ftxui::hbox({
                   HistoryPanel(state) | ftxui::flex,
                   SearchPanel(state) | ftxui::flex,
                   PersistencePanel(state) | ftxui::flex,
               }),
               ftxui::separator(),
               EventLogPanel(state),
           }) |
           ftxui::border;
  });

  auto root = ftxui::CatchEvent(renderer, [&](const ftxui::Event& e) {
    if (e == ftxui::Event::Escape) {
      screen.ExitLoopClosure()();
      return true;
    }
    if (e == ftxui::Event::F1) {
      state.history.Clear();
      state.evicted = 0;
      state.Log("history cleared");
      return true;
    }
    if (e == ftxui::Event::F2) {
      state.sensitive_mode = !state.sensitive_mode;
      if (state.sensitive_mode) {
        state.history.SetSensitivePolicy(std::make_shared<PrefixSensitivePolicy>("secret:"));
        state.Log("sensitive mode ON — 'secret:*' not persisted");
      } else {
        state.history.SetSensitivePolicy(std::make_shared<NeverSensitivePolicy>());
        state.Log("sensitive mode OFF");
      }
      return true;
    }
    if (e == ftxui::Event::F3) {
      state.prefix_search = !state.prefix_search;
      return true;
    }
    if (e == ftxui::Event::Tab) {
      if (command_input->Focused()) {
        search_input->TakeFocus();
      } else {
        command_input->TakeFocus();
      }
      return true;
    }
    return false;
  });

  screen.Loop(root);

  // Final summary printed on exit (still uses only the public API).
  std::cout << "History size=" << state.history.Size() << " capacity=" << state.history.Capacity()
            << " evicted=" << state.evicted << "\n";
  return 0;
}