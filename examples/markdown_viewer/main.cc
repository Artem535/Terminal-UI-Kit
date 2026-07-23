#include <memory>
#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/key_hint_bar.h"
#include "terminal_ui_kit/markdown/markdown_document.h"
#include "terminal_ui_kit/markdown/markdown_view.h"
#include "terminal_ui_kit/theme/theme.h"

int main() {
  using namespace terminal_ui_kit;

  const Theme& theme = default_dark_theme();

  const std::string kSampleMarkdown = R"(# Terminal UI Kit

A **C++20/23** library of *high-level components* for modern interactive
terminal applications, built on top of [FTXUI](https://github.com/ArthurSonzogni/FTXUI).

## Features

- Virtualized lists and documents
- Streaming output
- Markdown rendering
- Text selection
- Clipboard integration

## Example Code

```cpp
#include <ftxui/component/component.hpp>

auto list = VirtualList(options);
auto screen = ftxui::ScreenInteractive::Fullscreen();
screen.Loop(list);
```

## Supported Elements

1. Headings (h1-h6)
2. **Bold** and *italic* text
3. `Inline code` and code blocks
4. Unordered and ordered lists
5. Links and horizontal rules

---

> This is a blockquote with some important information.

For more details, visit the [GitHub repository](https://github.com/Artem535/Terminal-UI-Kit).
)";

  auto doc = std::make_shared<MarkdownDocument>(kSampleMarkdown);
  MarkdownViewOptions view_opts;
  view_opts.theme = theme;
  auto view = MarkdownView(doc, view_opts);

  auto screen = ftxui::ScreenInteractive::Fullscreen();

  ftxui::Component root = ftxui::Renderer(view, [&] {
    return ftxui::vbox({
               ftxui::text("Terminal UI Kit - Markdown Viewer") | ftxui::bold,
               ftxui::text("Sample Markdown rendering") | ftxui::dim,
               ftxui::separator(),
               view->Render() | ftxui::flex,
               ftxui::separator(),
               KeyHintBar({{"up/down", "scroll"}, {"q/ESC", "quit"}}, theme),
           }) |
           ftxui::border;
  });

  root |= ftxui::CatchEvent([&](const ftxui::Event& event) {
    if (event == ftxui::Event::Character('q') || event == ftxui::Event::Escape) {
      screen.ExitLoopClosure()();
      return true;
    }
    return false;
  });

  screen.Loop(root);
}