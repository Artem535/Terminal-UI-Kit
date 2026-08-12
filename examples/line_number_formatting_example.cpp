// Example: line-number gutter width (CodeView / VirtualDocument)
//
// Demonstrates the line-number gutter fix for unsigned underflow: a rendered
// line number wider than the configured gutter width is shown in full (never
// truncated) and produces no huge padding. The gutter is a plain text right-
// aligned to the width, so a number with more digits than the width simply has
// no leading padding.
//
// The document contains exactly enough lines so that the line numbers
// 1, 9, 99, 9999, 10000, 99999 and 100000 all exist. Cycling the gutter width
// (key 'w') shows: short numbers right-aligned; numbers wider than the gutter
// rendered in full; no clipping; and no enormous space after a wide number.
//
// Controls:
//   w        cycle the gutter width through 1, 2, 3, 4, 5, 6, 8
//   up/down  scroll
//   q / ESC  quit
//
// Build/run (CMake):
//   cmake --build --preset debug --target terminal_ui_kit_example_line_number_formatting
//   ./build/debug/examples/terminal_ui_kit_example_line_number_formatting

#include <cstddef>
#include <memory>
#include <string>

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

// Gutter widths to cycle through when demonstrating alignment.
constexpr int kWidths[] = {1, 2, 3, 4, 5, 6, 8};
constexpr int kWidthCount = static_cast<int>(sizeof(kWidths) / sizeof(kWidths[0]));

// The sample line numbers that must exist in the document.
constexpr int kSampleLines[] = {1, 9, 99, 9999, 10000, 99999, 100000};
constexpr int kSampleCount = static_cast<int>(sizeof(kSampleLines) / sizeof(kSampleLines[0]));

void BuildDocument(terminal_ui_kit::StreamingDocument& doc) {
  const int last = kSampleLines[kSampleCount - 1];  // 100000
  for (int i = 1; i <= last; ++i) {
    doc.append("Line " + std::to_string(i));
    for (int s = 0; s < kSampleCount; ++s) {
      if (i == kSampleLines[s]) {
        doc.append("  <-- sample line number");
      }
    }
    doc.append("\n");
  }
  doc.finish();
}

std::string SampleLineNumbers() {
  std::string out;
  for (int s = 0; s < kSampleCount; ++s) {
    if (s > 0) out += " ";
    out += std::to_string(kSampleLines[s]);
  }
  return out;
}

}  // namespace

int main() {
  using namespace terminal_ui_kit;
  const Theme& theme = default_dark_theme();

  StreamingDocument doc;
  BuildDocument(doc);

  int width_index = 4;  // start at width 5 (default); 6-digit lines are wider.
  std::unique_ptr<VirtualDocument> view;
  auto rebuild = [&] {
    VirtualDocumentOptions opts;
    opts.document = &doc;
    opts.theme = theme;
    opts.show_line_numbers = true;
    opts.line_number_width = static_cast<std::size_t>(kWidths[width_index]);
    opts.follow = true;
    view = std::make_unique<VirtualDocument>(std::move(opts));
  };
  rebuild();

  auto screen = ftxui::ScreenInteractive::Fullscreen();

  // Wrap the interactive view in an optional static CodeView panel that also
  // exercises the shared gutter logic through CodeView's plain-text path.
  CodeViewOptions code_opts;
  code_opts.show_line_numbers = true;
  code_opts.line_number_width = static_cast<std::size_t>(kWidths[width_index]);

  auto root = ftxui::Renderer(view->component(), [&] {
    const int width = kWidths[width_index];
    const std::string sample = SampleLineNumbers();
    const std::string status = "Gutter width: " + std::to_string(width) +
                               "   (digits wider than the "
                               "gutter render in full, never truncated)";
    return ftxui::vbox({
               ftxui::text("Terminal UI Kit - Line Number Gutter") | ftxui::bold,
               ftxui::text(sample) | ftxui::dim,
               ftxui::text("Line numbers present: " + std::to_string(kSampleCount)) | ftxui::dim,
               ftxui::separator(),
               ftxui::hbox({
                   view->component()->Render() | ftxui::flex,
                   ftxui::vbox({
                       ftxui::text("CodeView (plain path)") | ftxui::bold,
                       ftxui::separator(),
                       CodeView("int main() {\n  return 0;\n}", code_opts) | ftxui::flex,
                   }) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 40),
               }),
               ftxui::separator(),
               ftxui::text(status),
               KeyHintBar({{"w", "cycle width"}, {"up/down", "scroll"}, {"q/ESC", "quit"}}, theme),
           }) |
           ftxui::border;
  });

  root |= ftxui::CatchEvent([&](const ftxui::Event& event) {
    if (event == ftxui::Event::Character('w')) {
      width_index = (width_index + 1) % kWidthCount;
      code_opts.line_number_width = static_cast<std::size_t>(kWidths[width_index]);
      rebuild();
      return true;
    }
    if (event == ftxui::Event::Character('q') || event == ftxui::Event::Escape) {
      screen.ExitLoopClosure()();
      return true;
    }
    return false;
  });

  screen.Loop(root);
}