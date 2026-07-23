#include "terminal_ui_kit/components/code_view.h"

#include <string>
#include <vector>

#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/style_bridge.h"
#include "terminal_ui_kit/core/text_wrap.h"

namespace terminal_ui_kit {

ftxui::Element CodeView(std::string code, CodeViewOptions options) {
  ftxui::Elements lines;
  std::vector<std::string> wrapped = wrap_plain_text(code, 80);

  for (std::size_t i = 0; i < wrapped.size(); ++i) {
    ftxui::Elements parts;
    if (options.show_line_numbers) {
      std::string num = std::to_string(i + 1);
      num = std::string(4 - num.size(), ' ') + num;
      parts.push_back(ftxui::text(num) | ftxui::color(ftxui::Color::GrayDark));
      parts.push_back(ftxui::text(" "));
    }
    parts.push_back(ftxui::text(wrapped[i]) | to_decorator(options.theme.code));
    lines.push_back(ftxui::hbox(std::move(parts)));
  }

  ftxui::Element result = ftxui::vbox(std::move(lines));

  if (!options.language.empty()) {
    result = ftxui::vbox({
      ftxui::hbox({
        ftxui::text(" ") | to_decorator(options.theme.muted),
        ftxui::text(options.language) | to_decorator(options.theme.muted),
      }),
      result,
    });
  }

  return result;
}

}  // namespace terminal_ui_kit