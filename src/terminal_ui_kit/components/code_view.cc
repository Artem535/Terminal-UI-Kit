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
    // Render highlighted spans line by line, preserving per-span styles.
    // Each span is individually styled; we split at newlines and group
    // spans on the same line into an hbox so tokens keep their colour.
    std::size_t line_num = 1;
    ftxui::Elements current_line_parts;  // styled spans accumulated for the current line

    // Helper: flush one line from current_line_parts.
    auto flush_line = [&] {
      ftxui::Elements parts;
      if (options.show_line_numbers) {
        std::string num = std::to_string(line_num);
        num = std::string(4 - num.size(), ' ') + num;
        parts.push_back(ftxui::text(num) | ftxui::color(ftxui::Color::GrayDark));
        parts.push_back(ftxui::text(" "));
      }
      for (auto& el : current_line_parts) parts.push_back(std::move(el));
      lines.push_back(ftxui::hbox(std::move(parts)));
      current_line_parts.clear();
      ++line_num;
    };

    for (const auto& span : highlighted.spans()) {
      std::size_t pos = 0;
      while (pos < span.text.size()) {
        auto nl = span.text.find('\n', pos);
        if (nl == std::string::npos) {
          // No newline → push as one styled element.
          current_line_parts.push_back(ftxui::text(std::string(span.text.substr(pos))) |
                                       to_decorator(span.style));
          break;
        }
        // Text before the newline, if any, gets its own element.
        if (nl > pos) {
          current_line_parts.push_back(ftxui::text(std::string(span.text.substr(pos, nl - pos))) |
                                       to_decorator(span.style));
        }
        // The newline ends the current line.
        flush_line();
        pos = nl + 1;
      }
    }
    // Flush any remaining content (no trailing newline).
    if (!current_line_parts.empty()) {
      flush_line();
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