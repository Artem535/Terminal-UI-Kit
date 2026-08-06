// Line-number formatting example.
//
// Demonstrates the configurable line-number gutter of CodeView and
// VirtualDocument. The gutter width is switchable at runtime; the example
// shows how short line numbers are right-aligned, how numbers wider than the
// configured gutter render fully without clipping and without inserting
// enormous padding, and that a zero width is handled safely. All sample data
// is deterministic and local (no network or external service), the layout
// adapts to terminal resize, and rendering is fully virtualized for the
// 100000-line document.
//
// Controls:
//   [1]..[8]  set the line-number gutter width
//   [0]       set a zero gutter width
//   [q] / ESC quit
//
// Exit: press 'q' or ESC.

#include <cstddef>
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

// The sample document required by the benchmark: each line's content is a
// number of increasing width (1, 9, 99, 9999, 10000, 99999, 100000).
std::string BuildSampleDocument() {
  std::string doc;
  doc += "1\n";
  doc += "9\n";
  doc += "99\n";
  doc += "9999\n";
  doc += "10000\n";
  doc += "99999\n";
  doc += "100000\n";
  return doc;
}

// A larger document so a VirtualDocument's own gutter reaches line 100000 and
// therefore exceeds small configured gutter widths. VirtualDocument
// virtualizes rendering, so 100000 lines stay cheap.
std::string BuildLargeDocument(std::size_t line_count) {
  std::string doc;
  doc.reserve(line_count * 6);
  for (std::size_t i = 1; i <= line_count; ++i) {
    doc += "Line " + std::to_string(i);
    if (i < line_count) {
      doc += "\n";
    }
  }
  return doc;
}

}  // namespace

int main() {
  using namespace terminal_ui_kit;

  const Theme& theme = default_dark_theme();
  int width = 4;

  // Sample document (7 lines: 1, 9, 99, 9999, 10000, 99999, 100000) shown in
  // both CodeView and VirtualDocument.
  const std::string sample = BuildSampleDocument();

  StreamingDocument sample_doc;
  sample_doc.append(sample);
  sample_doc.finish();

  VirtualDocumentOptions sample_opts;
  sample_opts.document = &sample_doc;
  sample_opts.theme = theme;
  sample_opts.show_line_numbers = true;
  sample_opts.line_number_width = width;
  sample_opts.follow = false;
  VirtualDocument sample_view(std::move(sample_opts));

  // Large document (100000 lines) to demonstrate a gutter line number wider
  // than the configured width.
  const std::string large = BuildLargeDocument(100000);

  StreamingDocument large_doc;
  large_doc.append(large);
  large_doc.finish();

  VirtualDocumentOptions large_opts;
  large_opts.document = &large_doc;
  large_opts.theme = theme;
  large_opts.show_line_numbers = true;
  large_opts.line_number_width = width;
  large_opts.follow = false;
  VirtualDocument large_view(std::move(large_opts));
  large_view.scroll_to_bottom();

  auto screen = ftxui::ScreenInteractive::Fullscreen();

  auto root = ftxui::Renderer([&] {
    // The virtual list needs a laid-out box before scroll_to_bottom can target
    // the true bottom; re-request it each frame (the demo's focus is the
    // bottom of the 100000-line document).
    large_view.scroll_to_bottom();
    sample_opts.line_number_width = width;
    large_opts.line_number_width = width;

    CodeViewOptions code_opts;
    code_opts.show_line_numbers = true;
    code_opts.line_number_width = width;
    code_opts.theme = theme;

    return ftxui::vbox({
               ftxui::text("Terminal UI Kit - Line Number Formatting") | ftxui::bold,
               ftxui::text("Switch gutter width with [1]-[8] (0 = zero width). "
                           "Numbers wider than the gutter render fully, without "
                           "huge spaces or clipping.") |
                   ftxui::dim,
               ftxui::separator(),
               ftxui::text("CodeView (document: 1, 9, 99, 9999, 10000, 99999, "
                           "100000)") |
                   ftxui::color(ftxui::Color::GrayDark),
               ftxui::hbox({
                   ftxui::text("CodeView:") | ftxui::bold |
                       ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 10),
                   CodeView(sample, code_opts) | ftxui::flex,
               }),
               ftxui::separator(),
               ftxui::text("VirtualDocument (sample): top of document, gutter 1..7") |
                   ftxui::color(ftxui::Color::GrayDark),
               sample_view.component()->Render() | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 7),
               ftxui::separator(),
               ftxui::text("VirtualDocument (100000 lines): bottom, gutter "
                           "reaches 100000") |
                   ftxui::color(ftxui::Color::GrayDark),
               large_view.component()->Render() | ftxui::flex,
               ftxui::separator(),
               ftxui::text(std::string("Gutter width: ") + std::to_string(width)) | ftxui::bold,
               KeyHintBar({{"1-8", "set width"}, {"0", "zero width"}, {"q/ESC", "quit"}}, theme),
           }) |
           ftxui::border;
  });

  root |= ftxui::CatchEvent([&](const ftxui::Event& event) {
    if (event == ftxui::Event::Character('q') || event == ftxui::Event::Escape) {
      screen.ExitLoopClosure()();
      return true;
    }
    if (event == ftxui::Event::Character('0')) {
      width = 0;
      return true;
    }
    for (char c = '1'; c <= '8'; ++c) {
      if (event == ftxui::Event::Character(c)) {
        width = c - '0';
        return true;
      }
    }
    return false;
  });

  screen.Loop(root);
}