// Line Number Formatting example.
//
// Demonstrates the line-number gutter of CodeView and VirtualDocument: how
// numbers shorter than the configured gutter width are right-aligned, numbers
// equal to the width render unpadded, and numbers longer than the width render
// in full without truncation and without an enormous padding run (the fixed
// unsigned-underflow bug). The document has 100000 lines so the gutter shows
// numbers from 1 digit up to 6 digits (1, 9, 99, 9999, 10000, 99999, 100000).
//
// Controls:
//   + / -      increase / decrease the gutter width
//   1 .. 9     set the gutter width directly
//   q / ESC    quit
//
// The gutter width is a construction-time option for both views, so the
// cached CodeView element and the VirtualDocument component are rebuilt only
// when the width changes; terminal resizes re-render the cached trees without
// rebuilding them.

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/code_view.h"
#include "terminal_ui_kit/components/key_hint_bar.h"
#include "terminal_ui_kit/components/virtual_document.h"
#include "terminal_ui_kit/document/streaming_document.h"
#include "terminal_ui_kit/theme/theme.h"

namespace {

constexpr std::size_t kLineCount = 100000;

// Deterministic document: "Line 1\nLine 2\n...\nLine 100000\n". The line-number
// gutter consequently shows 1-digit through 6-digit numbers.
std::string BuildDocument() {
  std::string doc;
  doc.reserve(kLineCount * 9);
  for (std::size_t i = 1; i <= kLineCount; ++i) {
    doc += "Line " + std::to_string(i) + "\n";
  }
  return doc;
}

struct AppState {
  std::size_t gutter_width = 5;
  bool rebuild_pending = true;
  std::shared_ptr<terminal_ui_kit::VirtualDocument> virtual_view;
  ftxui::Element code_view;
};

}  // namespace

int main() {
  using namespace terminal_ui_kit;

  const std::string code = BuildDocument();
  const Theme& theme = default_dark_theme();

  StreamingDocument doc;
  doc.append(code);
  doc.finish();

  auto screen = ftxui::ScreenInteractive::Fullscreen();
  AppState state;

  auto rebuild = [&] {
    CodeViewOptions copts;
    copts.show_line_numbers = true;
    copts.gutter_width = state.gutter_width;
    state.code_view = CodeView(code, std::move(copts));

    VirtualDocumentOptions vopts;
    vopts.document = &doc;
    vopts.show_line_numbers = true;
    vopts.gutter_width = state.gutter_width;
    state.virtual_view = std::make_shared<VirtualDocument>(std::move(vopts));
  };
  rebuild();

  auto renderer = ftxui::Renderer([&] {
    if (state.rebuild_pending) {
      rebuild();
      state.rebuild_pending = false;
    }
    return ftxui::vbox({
               ftxui::text("Terminal UI Kit - Line Number Formatting") | ftxui::bold,
               ftxui::text("Gutter width: " + std::to_string(state.gutter_width) +
                           "   line numbers: 1, 9, 99, 9999, 10000, 99999, 100000") |
                   ftxui::dim,
               ftxui::separator(),
               ftxui::hbox({
                   ftxui::vbox({
                       ftxui::text("CodeView") | ftxui::bold,
                       state.code_view | ftxui::flex,
                   }) | ftxui::border |
                       ftxui::flex,
                   ftxui::vbox({
                       ftxui::text("VirtualDocument") | ftxui::bold,
                       state.virtual_view->component()->Render() | ftxui::flex,
                   }) | ftxui::border |
                       ftxui::flex,
               }) | ftxui::flex,
               ftxui::separator(),
               KeyHintBar({{"+/-", "gutter width"}, {"1..9", "set width"}, {"q/ESC", "quit"}},
                          theme),
           }) |
           ftxui::border;
  });

  renderer |= ftxui::CatchEvent([&](const ftxui::Event& event) {
    if (event == ftxui::Event::Character('q') || event == ftxui::Event::Escape) {
      screen.ExitLoopClosure()();
      return true;
    }
    if (event == ftxui::Event::Character('+') || event == ftxui::Event::Character('=')) {
      if (state.gutter_width < 20) {
        ++state.gutter_width;
        state.rebuild_pending = true;
      }
      return true;
    }
    if (event == ftxui::Event::Character('-') || event == ftxui::Event::Character('_')) {
      if (state.gutter_width > 0) {
        --state.gutter_width;
        state.rebuild_pending = true;
      }
      return true;
    }
    const std::string input = event.input();
    if (input.size() == 1 && input[0] >= '1' && input[0] <= '9') {
      state.gutter_width = static_cast<std::size_t>(input[0] - '0');
      state.rebuild_pending = true;
      return true;
    }
    return false;
  });

  screen.Loop(renderer);
  return 0;
}