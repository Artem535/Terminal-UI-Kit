#include "terminal_ui_kit/components/code_view.h"

#include <string>
#include <vector>

#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/style_bridge.h"
#include "terminal_ui_kit/core/styled_text.h"
#include "terminal_ui_kit/core/text_wrap.h"

#ifdef TERMINAL_UI_KIT_ENABLE_TREE_SITTER
#include "terminal_ui_kit/syntax/syntax_highlighter.h"
#endif

namespace terminal_ui_kit {

ftxui::Element CodeView(std::string code, CodeViewOptions options) {
  ftxui::Elements lines;

  bool use_highlighting = false;
  StyledText highlighted;

#ifdef TERMINAL_UI_KIT_ENABLE_TREE_SITTER
  if (!options.language.empty()) {
    highlighted = SyntaxHighlighter::highlight(code, options.language, options.theme);
    use_highlighting = true;
  }
#endif

  if (use_highlighting && !highlighted.spans().empty()) {
    // Render highlighted spans line by line
    std::string current_line;
    TextStyle current_style;
    std::size_t line_num = 1;

    for (const auto& span : highlighted.spans()) {
      std::size_t pos = 0;
      while (pos < span.text.size()) {
        auto nl = span.text.find('\n', pos);
        if (nl == std::string::npos) {
          current_line += span.text.substr(pos);
          current_style = span.style;
          break;
        }
        current_line += span.text.substr(pos, nl - pos);
        // Flush line
        ftxui::Elements parts;
        if (options.show_line_numbers) {
          std::string num = std::to_string(line_num);
          num = std::string(4 - num.size(), ' ') + num;
          parts.push_back(ftxui::text(num) | ftxui::color(ftxui::Color::GrayDark));
          parts.push_back(ftxui::text(" "));
        }
        parts.push_back(ftxui::text(current_line) | to_decorator(current_style));
        lines.push_back(ftxui::hbox(std::move(parts)));
        current_line.clear();
        ++line_num;
        pos = nl + 1;
      }
    }
    if (!current_line.empty()) {
      ftxui::Elements parts;
      if (options.show_line_numbers) {
        std::string num = std::to_string(line_num);
        num = std::string(4 - num.size(), ' ') + num;
        parts.push_back(ftxui::text(num) | ftxui::color(ftxui::Color::GrayDark));
        parts.push_back(ftxui::text(" "));
      }
      parts.push_back(ftxui::text(current_line) | to_decorator(current_style));
      lines.push_back(ftxui::hbox(std::move(parts)));
    }
  } else {
    // Plain text fallback
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