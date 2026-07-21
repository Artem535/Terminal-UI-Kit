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
    case terminal_ui_kit::ProgressStyle::kUnicodeBlocks: return "Unicode blocks";
    case terminal_ui_kit::ProgressStyle::kAscii: return "ASCII";
    case terminal_ui_kit::ProgressStyle::kDots: return "Dots";
    case terminal_ui_kit::ProgressStyle::kBraille: return "Braille";
  }
  return "Unknown";
}

}  // namespace

int main() {
  using namespace terminal_ui_kit;
  const Theme& theme = default_dark_theme();
  const std::array styles = {ProgressStyle::kUnicodeBlocks, ProgressStyle::kAscii,
                             ProgressStyle::kDots, ProgressStyle::kBraille};
  std::size_t style_index = 0;
  double fraction = 0.5;
  ProgressBarOptions options;
  options.width = 20;
  ftxui::Component indeterminate = IndeterminateProgress(theme, options);
  ftxui::Component root = ftxui::Renderer(indeterminate, [&] {
    options.style = styles[style_index];
    return ftxui::vbox({
               ftxui::text("Terminal UI Kit - Progress Viewer") | ftxui::bold,
               ftxui::text("Interactive tour of determinate and animated progress.") | ftxui::dim,
               ftxui::separator(),
               ftxui::text("ProgressBar") | ftxui::bold,
               ftxui::text("Known completion: " + std::string(style_name(options.style)) +
                           " style at a user-controlled percentage.") |
                   ftxui::dim,
               ProgressBar(fraction, theme, options),
               ftxui::separator(),
               ftxui::text("IndeterminateProgress") | ftxui::bold,
               ftxui::text("Unknown completion: a wrapping animated segment.") | ftxui::dim,
               indeterminate->Render(),
               ftxui::separator(),
               ftxui::text("Controls") | ftxui::bold,
               ftxui::filler(),
               KeyHintBar({{"left/right", "change progress"}, {"s", "cycle style"}, {"q", "quit"}}, theme),
           }) |
           ftxui::border;
  });
  auto screen = ftxui::ScreenInteractive::Fullscreen();
  root |= ftxui::CatchEvent([&](ftxui::Event event) {
    if (event == ftxui::Event::ArrowLeft) fraction = std::max(0.0, fraction - 0.05);
    else if (event == ftxui::Event::ArrowRight) fraction = std::min(1.0, fraction + 0.05);
    else if (event == ftxui::Event::Character('s')) style_index = (style_index + 1) % styles.size();
    else if (event == ftxui::Event::Character('q')) screen.ExitLoopClosure()();
    else return false;
    return true;
  });
  screen.Loop(root);
}
