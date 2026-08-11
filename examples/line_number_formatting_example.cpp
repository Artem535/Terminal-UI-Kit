// Example: line-number gutter formatting
//
// Demonstrates right-aligned line-number gutters for both CodeView and
// VirtualDocument, and the shared core format_line_number helper. It shows the
// required document (lines 1, 9, 99, 9999, 10000, 99999, 100000) under several
// configurable gutter widths, including widths smaller than the digit count,
// to show that long numbers are never truncated, never underflow, and never
// produce enormous padding.
//
// Controls:
//   Q / Esc  — quit
//   + / =    — increase gutter width
//   -        — decrease gutter width
//   c        — toggle CodeView vs VirtualDocument view
//
// The example is deterministic, needs no network access, and re-renders
// correctly after terminal resize.

#include <cstddef>
#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/code_view.h"
#include "terminal_ui_kit/components/virtual_document.h"
#include "terminal_ui_kit/core/line_number.h"
#include "terminal_ui_kit/document/streaming_document.h"
#include "terminal_ui_kit/theme/theme.h"

namespace {

constexpr char kDocument[] =
    "one\n"
    "nine\n"
    "ninety-nine\n"
    "line 9999\n"
    "line 10000\n"
    "line 99999\n"
    "line 100000\n";

}  // namespace

int main() {
  using namespace terminal_ui_kit;

  const Theme& theme = default_dark_theme();

  std::size_t gutter_width = 5;
  bool show_code_view = true;

  StreamingDocument doc;
  doc.append(kDocument);
  doc.finish();

  VirtualDocumentOptions vopts;
  vopts.document = &doc;
  vopts.theme = theme;
  vopts.show_line_numbers = true;
  vopts.follow = false;
  VirtualDocument view(std::move(vopts));

  auto screen = ftxui::ScreenInteractive::Fullscreen();

  auto root = ftxui::Renderer([&] {
    ftxui::Element body;
    if (show_code_view) {
      CodeViewOptions copts;
      copts.show_line_numbers = true;
      copts.theme = theme;
      body = CodeView(kDocument, copts);
    } else {
      body = view.component()->Render();
    }

    // Manual gutter-preview using the shared helper for the required numbers.
    ftxui::Elements preview;
    const std::size_t nums[] = {1, 9, 99, 9999, 10000, 99999, 100000};
    for (std::size_t n : nums) {
      preview.push_back(ftxui::text(format_line_number(n, gutter_width)) |
                        ftxui::color(ftxui::Color::GrayDark));
    }

    return ftxui::vbox({
               ftxui::text("Line-number gutter formatting") | ftxui::bold,
               ftxui::text("Gutter width: " + std::to_string(gutter_width) +
                           "   view: " + (show_code_view ? "CodeView" : "VirtualDocument")) |
                   ftxui::dim,
               ftxui::separator(),
               ftxui::hbox({
                   ftxui::vbox(preview) | ftxui::border,
                   body | ftxui::flex,
               }) | ftxui::flex,
               ftxui::separator(),
               ftxui::text(
                   "Controls: [+/-] gutter width   [c] CodeView/VirtualDocument   [q/Esc] quit") |
                   ftxui::dim,
           }) |
           ftxui::border;
  });

  root |= ftxui::CatchEvent([&](const ftxui::Event& event) {
    if (event == ftxui::Event::Character('q') || event == ftxui::Event::Escape) {
      screen.ExitLoopClosure()();
      return true;
    }
    if (event == ftxui::Event::Character('+') || event == ftxui::Event::Character('=')) {
      ++gutter_width;
      return true;
    }
    if (event == ftxui::Event::Character('-')) {
      if (gutter_width > 0) --gutter_width;
      return true;
    }
    if (event == ftxui::Event::Character('c')) {
      show_code_view = !show_code_view;
      return true;
    }
    return false;
  });

  screen.Loop(root);
  return 0;
}
