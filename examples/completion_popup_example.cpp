// CompletionPopup example application.
//
// Demonstrates the CompletionPopup component through an input field with two
// selectable providers:
//   - Immediate: a synchronous local provider (SyncCompletionProvider).
//   - Delayed:   a deterministic asynchronous provider that resolves after a
//                fixed delay, showing the loading state and the component's
//                stale-result protection when you type quickly.
//
// The example uses only the component's public API, needs no network, and uses
// a fixed sample dataset. All controls are shown in the UI.
//
// Exit: press 'q'. Toggle provider mode with 'm'.

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

#include "terminal_ui_kit/components/completion_popup.h"
#include "terminal_ui_kit/components/key_hint_bar.h"
#include "terminal_ui_kit/theme/theme.h"

namespace {

using terminal_ui_kit::CompletionContext;
using terminal_ui_kit::CompletionItem;
using terminal_ui_kit::CompletionKind;
using terminal_ui_kit::CompletionPopup;
using terminal_ui_kit::CompletionResult;
using terminal_ui_kit::ICompletionProvider;

// A fixed sample "standard library" used by both providers.
std::vector<CompletionItem> BuildDataset() {
  std::vector<CompletionItem> items;
  const auto add = [&items](std::string label, std::string category, std::string description,
                            CompletionKind kind, std::string insert = {}) {
    CompletionItem item;
    item.label = std::move(label);
    item.category = std::move(category);
    item.description = std::move(description);
    item.kind = kind;
    item.insert_text = std::move(insert);
    items.push_back(std::move(item));
  };
  add("connect", "net", "open a connection to a peer", CompletionKind::kFunction);
  add("connect_timeout", "net", "open a connection with a timeout", CompletionKind::kFunction);
  add("accept", "net", "accept an incoming connection", CompletionKind::kFunction);
  add("read_bytes", "io", "read raw bytes from a stream", CompletionKind::kFunction);
  add("write_bytes", "io", "write raw bytes to a stream", CompletionKind::kFunction);
  add("open", "io", "open a file descriptor", CompletionKind::kFunction);
  add("close", "io", "close a resource", CompletionKind::kFunction);
  add("json_parse", "json", "parse JSON text into a value", CompletionKind::kFunction);
  add("json_stringify", "json", "serialize a value to JSON", CompletionKind::kFunction);
  add("map", "collection", "transform every element", CompletionKind::kFunction);
  add("filter", "collection", "keep matching elements", CompletionKind::kFunction);
  add("reduce", "collection", "fold elements into one value", CompletionKind::kFunction);
  add("vector", "collection", "dynamic array type", CompletionKind::kType);
  add("string_view", "core", "non-owning string reference type", CompletionKind::kType);
  add("string", "core", "owning string type", CompletionKind::kType);
  add("execute", "sql", "run a SQL statement", CompletionKind::kFunction);
  add("commit", "sql", "commit the current transaction", CompletionKind::kFunction);
  add("rollback", "sql", "undo the current transaction", CompletionKind::kFunction);
  add("send", "http", "send an HTTP request", CompletionKind::kFunction, "send_request");
  add("render", "view", "render a scene", CompletionKind::kFunction);
  add("update", "view", "invalidate and redraw", CompletionKind::kFunction);
  return items;
}

// The trailing word of the input buffer is what we ask the provider to complete.
std::string TrailingWord(const std::string& buffer) {
  const std::size_t space = buffer.find_last_of(" \t\n");
  return space == std::string::npos ? buffer : buffer.substr(space + 1);
}

// Provider that runs either synchronously (immediate mode) or with a fixed
// delay (delayed mode). In delayed mode an older pending request is not dropped
// by the provider: it is delivered with its (now stale) generation, and the
// popup discards it, which is exactly the stale-result protection to show.
class DemoProvider : public ICompletionProvider {
 public:
  enum class Mode { kImmediate, kDelayed };

  DemoProvider(std::vector<CompletionItem> dataset, std::chrono::milliseconds delay)
      : dataset_(std::move(dataset)), delay_(delay) {}

  void set_mode(Mode mode) { mode_ = mode; }
  Mode mode() const { return mode_; }
  const char* mode_name() const { return mode_ == Mode::kImmediate ? "immediate" : "delayed"; }

  void complete(const CompletionContext& context, std::uint64_t generation,
                const std::function<void(std::uint64_t, CompletionResult)>& deliver) override {
    if (mode_ == Mode::kImmediate) {
      CompletionResult result;
      result.items = dataset_;
      deliver(generation, std::move(result));
      return;
    }
    pending_.push_back({generation, context, deliver, std::chrono::steady_clock::now() + delay_});
  }

  // Deliver any pending requests whose delay has elapsed. Returns true when
  // something was delivered so the host knows results changed.
  bool Tick() {
    bool delivered = false;
    const auto now = std::chrono::steady_clock::now();
    auto it = pending_.begin();
    while (it != pending_.end()) {
      if (now < it->due) {
        ++it;
        continue;
      }
      CompletionResult result;
      result.items = dataset_;
      const std::uint64_t generation = it->generation;
      const auto deliver = it->deliver;
      it = pending_.erase(it);
      deliver(generation, std::move(result));
      delivered = true;
    }
    return delivered;
  }

 private:
  struct Pending {
    std::uint64_t generation;
    CompletionContext context;
    std::function<void(std::uint64_t, CompletionResult)> deliver;
    std::chrono::steady_clock::time_point due;
  };

  std::vector<CompletionItem> dataset_;
  std::chrono::milliseconds delay_;
  Mode mode_ = Mode::kImmediate;
  std::vector<Pending> pending_;
};

}  // namespace

int main() {
  using namespace terminal_ui_kit;
  using ftxui::text;

  auto dataset = BuildDataset();
  auto provider = std::make_shared<DemoProvider>(dataset, std::chrono::milliseconds(250));

  std::string buffer;
  std::string query;
  std::string last_accepted;
  std::size_t accept_count = 0;

  CompletionPopupOptions popup_options;
  popup_options.provider = provider;
  popup_options.on_accept = [&](const CompletionItem& item) {
    // Apply the item's insertion at the completion site (replacement-range
    // aware). The cursor is at the end of the buffer and the query is the
    // trailing word, so the default (no explicit range) path replaces exactly
    // the typed query.
    const std::string word = TrailingWord(buffer);
    buffer = CompletionPopup::ApplyReplacement(buffer, buffer.size(), word, item);
    last_accepted = item.label;
    ++accept_count;
  };

  CompletionPopup* popup_ptr = nullptr;

  ftxui::InputOption input_options;
  std::string placeholder = "type a prefix, e.g.  conn | rea | json | sq";
  input_options.on_change = [&] {
    query = TrailingWord(buffer);
    if (popup_ptr != nullptr) {
      popup_ptr->set_query(query, buffer.size());
    }
  };

  ftxui::Component input = ftxui::Input(&buffer, placeholder, input_options);
  CompletionPopup popup(input, popup_options);
  popup_ptr = &popup;
  ftxui::Component component = popup.component();

  auto screen = ftxui::ScreenInteractive::Fullscreen();
  std::atomic<bool> running = true;

  // Ticker thread: wakes the UI loop so the delayed provider can resolve even
  // while the user is not typing.
  std::thread ticker([&] {
    while (running) {
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
      screen.PostEvent(ftxui::Event::Custom);
    }
  });

  ftxui::Component content = ftxui::Renderer(component, [&] {
    // Viewport-aware placement, recomputed every frame so it stays correct
    // after a resize: below the input when there is room, above otherwise.
    const auto dims = ftxui::Terminal::Size();
    const int available_below = std::max(1, dims.dimy - 6);
    popup.set_available_space(available_below, 3);

    ftxui::Element input_element = component->Render();
    const std::string state_text = [&] {
      switch (popup.state()) {
        case CompletionPopup::State::kLoading:
          return std::string("loading (delayed provider)");
        case CompletionPopup::State::kResults:
          return std::string("results: ") + std::to_string(popup.items().size());
        case CompletionPopup::State::kNoResults:
          return std::string("no matches");
        case CompletionPopup::State::kError:
          return std::string("provider error");
        case CompletionPopup::State::kHidden:
          return std::string("idle");
      }
      return std::string();
    }();

    return ftxui::vbox({
               text("Terminal UI Kit - CompletionPopup") | ftxui::bold,
               text("Fuzzy autocomplete with synchronous & asynchronous providers.") | ftxui::dim,
               ftxui::separator(),
               ftxui::hbox({text("> "), input_element}) | ftxui::flex,
               ftxui::separator(),
               text("provider: " + std::string(provider->mode_name()) +
                    "  |  state: " + state_text) |
                   ftxui::dim,
               text("accepted: " + last_accepted + "  (" + std::to_string(accept_count) +
                    " total)") |
                   ftxui::dim,
               KeyHintBar({{"type", "fuzzy filter"},
                           {"up/down", "select"},
                           {"enter/tab", "accept"},
                           {"esc", "cancel"},
                           {"m", "toggle provider"},
                           {"q", "quit"}},
                          default_dark_theme()),
           }) |
           ftxui::border;
  });

  ftxui::Component root = ftxui::CatchEvent(content, [&](ftxui::Event event) {
    if (event == ftxui::Event::Custom) {
      provider->Tick();  // deliver delayed results on the UI thread
      return true;
    }
    if (event == ftxui::Event::Character('q')) {
      running = false;
      screen.Exit();
      return true;
    }
    if (event == ftxui::Event::Character('m')) {
      provider->set_mode(provider->mode() == DemoProvider::Mode::kImmediate
                             ? DemoProvider::Mode::kDelayed
                             : DemoProvider::Mode::kImmediate);
      // Re-run the current query through the newly selected provider.
      popup.set_query(query, buffer.size());
      return true;
    }
    return false;
  });

  screen.Loop(root);
  running = false;
  ticker.join();
}
