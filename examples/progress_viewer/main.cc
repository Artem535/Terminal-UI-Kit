#include <array>
#include <cstddef>
#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/indeterminate_progress.h"
#include "terminal_ui_kit/components/key_hint_bar.h"
#include "terminal_ui_kit/components/progress_bar.h"
#include "terminal_ui_kit/theme/theme.h"

namespace {

const char* style_name(terminal_ui_kit::ProgressStyle style) {
  switch (style) {
    case terminal_ui_kit::ProgressStyle::kUnicodeBlocks:
      return "Unicode blocks";
    case terminal_ui_kit::ProgressStyle::kAscii:
      return "ASCII";
    case terminal_ui_kit::ProgressStyle::kDots:
      return "Dots";
    case terminal_ui_kit::ProgressStyle::kBraille:
      return "Braille";
  }
  return "Unknown";
}

}  // namespace

int main() {
  using namespace terminal_ui_kit;
  const Theme& theme = default_dark_theme();
  const std::array styles = {ProgressStyle::kUnicodeBlocks, ProgressStyle::kAscii,
                             ProgressStyle::kDots, ProgressStyle::kBraille};
  double fraction = 0.5;
  ProgressBarOptions options;
  options.width = 20;
  std::array<ftxui::Component, 4> indeterminate;
  for (std::size_t index = 0; index < styles.size(); ++index) {
    options.style = styles[index];
    indeterminate[index] = IndeterminateProgress(theme, options);
  }
  ftxui::Component children = ftxui::Container::Vertical(
      {indeterminate[0], indeterminate[1], indeterminate[2], indeterminate[3]});
  ftxui::Component root = ftxui::Renderer(children, [&] {
    ftxui::Elements rows;
    for (std::size_t index = 0; index < styles.size(); ++index) {
      options.style = styles[index];
      rows.push_back(ftxui::text(style_name(options.style)) | ftxui::bold);
      rows.push_back(ProgressBar(fraction, theme, options));
      rows.push_back(indeterminate[index]->Render());
    }
    return ftxui::vbox({
               ftxui::text("Terminal UI Kit - Progress Viewer") | ftxui::bold,
               ftxui::text("Interactive tour of determinate and animated progress.") | ftxui::dim,
               ftxui::separator(),
               ftxui::text("ProgressBar") | ftxui::bold,
               ftxui::text("Known completion in every supported style.") | ftxui::dim,
               ftxui::text("IndeterminateProgress") | ftxui::bold,
               ftxui::text("Unknown completion: a wrapping animated segment.") | ftxui::dim,
               ftxui::vbox(std::move(rows)),
               ftxui::separator(),
               ftxui::text("Controls") | ftxui::bold,
               ftxui::filler(),
               KeyHintBar({{"left/right", "change progress"}, {"q", "quit"}}, theme),
           }) |
           ftxui::border;
  });
  auto screen = ftxui::ScreenInteractive::Fullscreen();
  root |= ftxui::CatchEvent([&](ftxui::Event event) {
    if (event == ftxui::Event::ArrowLeft)
      fraction = std::max(0.0, fraction - 0.05);
    else if (event == ftxui::Event::ArrowRight)
      fraction = std::min(1.0, fraction + 0.05);
    else if (event == ftxui::Event::Character('q'))
      screen.ExitLoopClosure()();
    else
      return false;
    return true;
  });
  screen.Loop(root);
}
