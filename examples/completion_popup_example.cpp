// Example: CompletionPopup
//
// A fullscreen FTXUI application demonstrating the reusable completion popup
// component. It shows an input field with two selectable providers:
//
//   1. Immediate local provider (completion_popup::SyncCompletionProvider).
//   2. Deterministic delayed provider (completion_popup::AsyncCompletionProvider)
//      whose work is delivered ~50ms later from a background thread -- no
//      network access, fully deterministic sample data.
//
// Demonstrates: fuzzy filtering, categories, descriptions, loading state,
// no-results state, acceptance (Enter/Tab), stale-result protection (typing
// faster than the delayed provider supersedes older requests), and
// viewport-aware above/below placement that adapts when the terminal resizes.
//
// Controls:
//   type              filter completions (fuzzy)
//   up / down         move the selection
//   enter / tab       accept the selected completion
//   escape            dismiss the popup
//   1                 switch to the immediate provider
//   2                 switch to the delayed provider
//   q                 quit

#include <atomic>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/completion_popup.h"

namespace {

using terminal_ui_kit::AsyncCompletionProvider;
using terminal_ui_kit::CompletionContext;
using terminal_ui_kit::CompletionItem;
using terminal_ui_kit::CompletionKind;
using terminal_ui_kit::CompletionPopup;
using terminal_ui_kit::CompletionState;
using terminal_ui_kit::ICompletionProvider;
using terminal_ui_kit::SyncCompletionProvider;

// Deterministic sample vocabulary, shared by both providers. No network.
std::vector<CompletionItem> BuildDataset() {
  std::vector<CompletionItem> items;
  const auto add = [&items](std::string label, std::string insert, std::string desc,
                            std::string category, CompletionKind kind) {
    items.push_back(
        {std::move(label), std::move(insert), std::move(desc), std::move(category), kind, {}, {}});
  };

  add("int", "int", "Signed integer type", "Type", CompletionKind::kType);
  add("float", "float", "Single-precision floating point", "Type", CompletionKind::kType);
  add("double", "double", "Double-precision floating point", "Type", CompletionKind::kType);
  add("char", "char", "Single character", "Type", CompletionKind::kType);

  add("printf", "printf(", "Formatted output to stdout", "IO", CompletionKind::kFunction);
  add("fprintf", "fprintf(", "Formatted output to a stream", "IO", CompletionKind::kFunction);
  add("sprintf", "sprintf(", "Formatted output to a buffer", "IO", CompletionKind::kFunction);
  add("stdin", "stdin", "Standard input stream", "IO", CompletionKind::kVariable);
  add("stdout", "stdout", "Standard output stream", "IO", CompletionKind::kVariable);

  add("main", "int main(int argc, char** argv)", "Program entry point", "Signature",
      CompletionKind::kSnippet);
  add("for", "for (;;) {}", "For loop", "Keyword", CompletionKind::kKeyword);
  add("while", "while () {}", "While loop", "Keyword", CompletionKind::kKeyword);
  add("return", "return 0;", "Return from function", "Keyword", CompletionKind::kKeyword);

  // Duplicate label on purpose: two distinct entries share the label "id".
  add("id", "identifier", "A variable identifier", "Local", CompletionKind::kVariable);
  add("id", "identifier()", "Get the element identifier", "API", CompletionKind::kFunction);
  return items;
}

// A tiny deterministic delayed scheduler: work is queued and replayed ~50ms
// later from a background thread, so the async provider genuinely completes
// off the UI thread while remaining fully deterministic (no network).
class DelayedRunner {
 public:
  ~DelayedRunner() { Stop(); }

  void Start() {
    running_ = true;
    thread_ = std::thread([this] {
      while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::vector<std::function<void()>> batch;
        {
          std::lock_guard<std::mutex> lock(mutex_);
          batch.swap(queue_);
        }
        for (std::function<void()>& fn : batch) {
          fn();
        }
      }
    });
  }

  void Stop() {
    running_ = false;
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  void Schedule(std::function<void()> fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(std::move(fn));
  }

 private:
  std::atomic<bool> running_{false};
  std::mutex mutex_;
  std::vector<std::function<void()>> queue_;
  std::thread thread_;
};

std::string ProviderName(bool delayed) { return delayed ? "2) delayed" : "1) immediate"; }

std::string StateName(CompletionState state) {
  switch (state) {
    case CompletionState::kHidden:
      return "hidden";
    case CompletionState::kLoading:
      return "loading";
    case CompletionState::kResults:
      return "results";
    case CompletionState::kNoResults:
      return "no-results";
    case CompletionState::kError:
      return "error";
  }
  return "?";
}

}  // namespace

int main() {
  using namespace ftxui;

  std::string input_text;
  std::string placeholder = "type to trigger completion (fuzzy)";

  const std::vector<CompletionItem> dataset = BuildDataset();
  std::shared_ptr<ICompletionProvider> immediate = std::make_shared<SyncCompletionProvider>(
      [dataset](const CompletionContext&) { return dataset; });

  DelayedRunner runner;
  std::shared_ptr<ICompletionProvider> delayed = std::make_shared<AsyncCompletionProvider>(
      [dataset](const CompletionContext&) { return dataset; },
      [&runner](std::function<void()> fn) { runner.Schedule(std::move(fn)); });
  runner.Start();

  bool delayed_provider = false;
  std::string last_accepted;

  auto popup_model = std::make_shared<terminal_ui_kit::CompletionPopupModel>(immediate);
  popup_model->set_on_accept([&](const CompletionItem& item, const std::string& new_text, int) {
    input_text = new_text;
    last_accepted = item.label;
  });

  InputOption input_option;
  input_option.placeholder = placeholder;
  input_option.multiline = false;
  input_option.on_change = [&] {
    // The delayed provider can lag behind; the popup discards stale results.
    popup_model->set_input(input_text, static_cast<int>(input_text.size()));
  };

  int anchor_row = 4;
  auto screen = ScreenInteractive::Fullscreen();
  auto input = Input(&input_text, &placeholder, input_option);

  auto root = Renderer(input, [&] {
    const int viewport_height = screen.dimy();
    popup_model->set_anchor_row(anchor_row);
    popup_model->set_viewport_height(viewport_height);

    const std::string state_str = "State: " + StateName(popup_model->state());
    const std::string matches_str = "Query: '" + popup_model->query() +
                                    "'  matches: " + std::to_string(popup_model->items().size());

    Element content = vbox({
                          text("Terminal UI Kit - Completion Popup") | bold,
                          separator(),
                          text("Query:") | bold,
                          input->Render() | border,
                          separator(),
                          hbox({text("Provider: "), text(ProviderName(delayed_provider)) | bold,
                                text("   [1] immediate   [2] delayed") | dim}),
                          text(state_str) | dim,
                          text(matches_str) | dim,
                          text("Last accepted: " + last_accepted) | dim,
                          filler(),
                          separator(),
                          text("Controls") | bold,
                          text("type to filter, up/down select, enter/tab accept, "
                               "esc dismiss, 1/2 provider, q quit") |
                              dim,
                      }) |
                      border;

    if (popup_model->state() != CompletionState::kHidden) {
      Element popup_element = popup_model->component()->Render();
      content = dbox({std::move(content), popup_element | clear_under});
    }
    return content;
  });

  root |= CatchEvent([&](Event event) {
    if (event == Event::Character('q')) {
      runner.Stop();
      screen.ExitLoopClosure()();
      return true;
    }
    if (event == Event::Character('1')) {
      delayed_provider = false;
      popup_model->set_provider(immediate);
      return true;
    }
    if (event == Event::Character('2')) {
      delayed_provider = true;
      popup_model->set_provider(delayed);
      return true;
    }
    // Popup navigation / acceptance / dismissal.
    if (popup_model->state() == CompletionState::kResults) {
      if (event == Event::ArrowDown || event == Event::ArrowUp || event == Event::Tab ||
          event == Event::Return || event == Event::Escape) {
        return popup_model->component()->OnEvent(event);
      }
    } else if (event == Event::Escape) {
      return popup_model->component()->OnEvent(event);
    }
    return false;
  });

  screen.Loop(root);
  runner.Stop();
  return 0;
}
