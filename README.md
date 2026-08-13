# Terminal UI Kit

[![CI](https://github.com/Artem535/Terminal-UI-Kit/actions/workflows/ci.yml/badge.svg)](https://github.com/Artem535/Terminal-UI-Kit/actions/workflows/ci.yml)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C.svg)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.21%2B-064F8C.svg)](https://cmake.org/)
[![License: MIT](https://img.shields.io/github/license/Artem535/Terminal-UI-Kit)](LICENSE)
[![Status: early development](https://img.shields.io/badge/status-early%20development-orange.svg)](#project-status)

[Русская версия](README_RU.md)

**Terminal UI Kit** is a C++20 component library for building modern,
data-heavy terminal applications on top of
[FTXUI](https://github.com/ArthurSonzogni/FTXUI).

It provides reusable components for the parts of terminal applications that
usually require substantial application-specific code: virtualized lists,
streaming documents, log views, progress dashboards, themed status components,
Markdown rendering, and syntax-highlighted code.

Terminal UI Kit returns ordinary `ftxui::Component` and `ftxui::Element`
objects. It does not replace FTXUI, own the event loop, or impose an application
architecture.

> [!IMPORTANT]
> Terminal UI Kit is under active development. The current source version is
> `0.1.0`, and the public API may change before the first stable release.
> Pin a specific commit when using the library in another project.

## Why Terminal UI Kit?

FTXUI provides an excellent foundation for terminal rendering and interaction.
Terminal UI Kit builds on that foundation with higher-level components intended
for applications such as:

- developer tools and coding agents;
- log viewers and process monitors;
- task runners and build dashboards;
- terminal-based editors and document viewers;
- administration consoles;
- interactive CLI frontends.

The goal is to make these components reusable, composable, themeable, and
independent of any particular application domain.

## Highlights

- **FTXUI-native API** — components integrate with existing FTXUI layouts,
  containers, renderers, decorators, and event handling.
- **Virtualized rendering** — display large collections without rendering every
  item on every frame.
- **Streaming text model** — incrementally append UTF-8 text received from
  subprocesses, agents, network streams, or background tasks.
- **Document and log views** — scrolling, follow mode, line numbers, selection,
  ANSI styling, timestamps, and severity levels.
- **Reusable application components** — progress trees, spinners, status
  indicators, collapsible panels, key-hint bars, and modal overlays.
- **Optional rich-text modules** — Markdown through `cmark-gfm` and syntax
  highlighting through Tree-sitter.
- **Modular CMake targets** — link only the parts required by the application.
- **Testing infrastructure** — unit tests, rendering tests, sanitizers, and
  benchmarks.

## Project status

Terminal UI Kit is currently an early-stage library. The repository already
contains working core, document, and component modules, while several larger
areas remain experimental or planned.

| Area | Status | Included functionality |
| --- | --- | --- |
| Core text model | Available | Text positions, ranges, styles, selection, and wrapping |
| Basic components | Available | Status indicators, spinners, key hints, panels, modals, and progress views |
| Virtual lists | Available | Fixed and estimated variable-height rows, selection, and programmatic scrolling |
| Streaming documents | Available | Incremental UTF-8 input, logical lines, revisions, and tail replacement |
| Document views | Available | Wrapped rendering, follow mode, line numbers, selection, and copy callbacks |
| Log views | Available | Structured log entries, severity, timestamps, ANSI text, and follow mode |
| Code rendering | Available | Code blocks with optional Tree-sitter highlighting |
| Markdown | Experimental | `cmark-gfm`-based document parsing and rendering |
| Syntax highlighting | Experimental | Optional Tree-sitter integration and language grammars |
| Editor | Planned | Multiline editing, completion, and editing commands |
| Diff views | Planned | Unified and side-by-side diff presentation |
| Terminal integrations | Planned | Capability detection, clipboard, OSC 52, and image backends |

The immediate focus is correctness, API stabilization, Unicode terminal-cell
handling, packaging, tests, and cross-platform validation.

## Requirements

- CMake 3.21 or newer;
- a C++20 compiler;
- Git, when dependencies are obtained through CMake `FetchContent`;
- a terminal supported by FTXUI.

CMake first tries to resolve dependencies with `find_package()`. If FTXUI is not
available locally, the project fetches the pinned FTXUI dependency
automatically.

Optional features may require:

- `cmark-gfm` for Markdown;
- Tree-sitter and language grammars for syntax highlighting;
- Chafa for image rendering;
- GoogleTest for tests;
- Google Benchmark for benchmarks.

## Quick start

Clone the repository and build the example applications:

```sh
git clone https://github.com/Artem535/Terminal-UI-Kit.git
cd Terminal-UI-Kit

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DTERMINAL_UI_KIT_BUILD_EXAMPLES=ON

cmake --build build --parallel
```

Run the component gallery:

```sh
./build/examples/components_gallery/terminal_ui_kit_example_components_gallery
```

Other useful examples include:

```sh
./build/examples/virtual_list_viewer/terminal_ui_kit_example_virtual_list_viewer
./build/examples/virtual_document_viewer/terminal_ui_kit_example_virtual_document_viewer
./build/examples/streaming_log_viewer/terminal_ui_kit_example_streaming_log_viewer
./build/examples/progress_viewer/terminal_ui_kit_example_progress_viewer
./build/examples/task_dashboard/terminal_ui_kit_example_task_dashboard
./build/examples/command_history_example/terminal_ui_kit_example_command_history
```

## Basic usage

The following example displays a virtualized list containing 100,000 rows:

```cpp
#include <cstddef>
#include <string>

#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/virtual_list.h"

int main() {
  using namespace terminal_ui_kit;

  constexpr std::size_t kItemCount = 100'000;

  VirtualListOptions options;
  options.item_count = [] {
    return kItemCount;
  };
  options.render_item = [](std::size_t index, int /* width */) {
    return ftxui::text("Row " + std::to_string(index));
  };
  options.on_select = [](std::size_t index) {
    // Handle the selected row.
  };

  auto list = VirtualList(std::move(options));
  auto screen = ftxui::ScreenInteractive::Fullscreen();
  screen.Loop(list);
}
```

Link the application with the components module:

```cmake
target_link_libraries(my_application PRIVATE
  TerminalUiKit::Components
  ftxui::screen
)
```

## Streaming documents

`StreamingDocument` is designed for text that arrives incrementally, including
output from subprocesses, LLM agents, build systems, and remote services.

```cpp
#include "terminal_ui_kit/components/virtual_document.h"
#include "terminal_ui_kit/document/streaming_document.h"

terminal_ui_kit::StreamingDocument document;

terminal_ui_kit::VirtualDocumentOptions options;
options.document = &document;
options.follow = true;
options.show_line_numbers = true;

terminal_ui_kit::VirtualDocument view(std::move(options));

// Chunks may end in the middle of a line or a UTF-8 sequence.
document.append("Starting task...\n");
document.append("Processing item ");
document.append("42\n");
```

After modifying a document from a worker thread, notify the FTXUI event loop
with `ScreenInteractive::PostEvent(ftxui::Event::Custom)` and use appropriate
synchronization for shared state.

## Using Terminal UI Kit with FetchContent

Until stable releases and package-manager ports are available, the recommended
integration method is CMake `FetchContent` with a pinned tag or commit:

```cmake
include(FetchContent)

FetchContent_Declare(
  terminal_ui_kit
  GIT_REPOSITORY https://github.com/Artem535/Terminal-UI-Kit.git
  GIT_TAG        <pinned-commit-or-release>
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(terminal_ui_kit)

target_link_libraries(my_application PRIVATE
  TerminalUiKit::Components
  TerminalUiKit::Document
  ftxui::screen
)
```

Do not use the moving `main` branch for reproducible builds.

## CMake targets

The primary public targets are:

| Target | Purpose |
| --- | --- |
| `TerminalUiKit::Core` | Text model, styles, selection, wrapping, and themes |
| `TerminalUiKit::Document` | Streaming documents, ANSI parsing, logs, and wrapped documents |
| `TerminalUiKit::Components` | FTXUI components and document views |
| `TerminalUiKit::Markdown` | Optional Markdown parsing and rendering |
| `TerminalUiKit::Syntax` | Optional Tree-sitter syntax highlighting |
| `TerminalUiKit::Testing` | Rendering-test helpers |
| `TerminalUiKit::All` | Convenience target for all modules |

The editor, diff, terminal, and image targets are reserved for the corresponding
modules as they are implemented.

## CMake options

All optional features are disabled by default.

| Option | Description |
| --- | --- |
| `TERMINAL_UI_KIT_BUILD_TESTS` | Build the test suite |
| `TERMINAL_UI_KIT_BUILD_EXAMPLES` | Build example applications |
| `TERMINAL_UI_KIT_BUILD_BENCHMARKS` | Build benchmarks |
| `TERMINAL_UI_KIT_BUILD_SHARED` | Build shared libraries |
| `TERMINAL_UI_KIT_ENABLE_MARKDOWN` | Enable Markdown support |
| `TERMINAL_UI_KIT_ENABLE_TREE_SITTER` | Enable Tree-sitter highlighting |
| `TERMINAL_UI_KIT_ENABLE_IMAGES` | Enable image-rendering support |
| `TERMINAL_UI_KIT_ENABLE_CHAFA` | Enable the Chafa image backend |
| `TERMINAL_UI_KIT_ENABLE_CLIPBOARD` | Enable native clipboard integration |
| `TERMINAL_UI_KIT_ENABLE_OSC52` | Enable OSC 52 clipboard integration |
| `TERMINAL_UI_KIT_ENABLE_SANITIZERS` | Build with ASan and UBSan |
| `TERMINAL_UI_KIT_ENABLE_CLANG_TIDY` | Run `clang-tidy` during the build |
| `TERMINAL_UI_KIT_WARNINGS_AS_ERRORS` | Treat compiler warnings as errors |

For a full development build:

```sh
cmake --preset all-features
cmake --build --preset all-features
```

Some optional modules and platform integrations are still under development;
the default and `debug` presets are the recommended starting points.

## Tests

Build and run the default test configuration:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Run the sanitizer configuration:

```sh
cmake --preset sanitizers
cmake --build --preset sanitizers
ctest --preset sanitizers
```

## Examples

The repository contains focused example applications for implemented
components:

- `components_gallery` — status components, panels, modals, themes, and code
  rendering;
- `theme_viewer` — semantic theme roles;
- `progress_viewer` — determinate and indeterminate progress;
- `task_dashboard` — hierarchical task state;
- `virtual_list_viewer` — a virtualized 100,000-row list;
- `streaming_log_viewer` — live structured logs with ANSI styling;
- `virtual_document_viewer` — incrementally updated wrapped text;
- `markdown_viewer` — Markdown rendering when the Markdown feature is enabled;
- `command_history_example` — bounded, navigable command history with search,
  persistence, and a sensitive-command policy.

Additional example directories may exist as placeholders for planned modules.
Only examples registered in [`examples/CMakeLists.txt`](examples/CMakeLists.txt)
are built.

## Documentation

The full documentation is stored as AsciiDoc under
[`docs/`](docs/modules/ROOT/pages/index.adoc) and can be built with
[Antora](https://antora.org/):

```sh
npm ci
npm run docs:build
```

The generated site is written to:

```text
build/site/index.html
```

Project planning and architectural material is available in
[`doc/`](doc/), including the product requirements and roadmap.

## Design principles

Terminal UI Kit follows a small set of architectural rules:

1. **Compose instead of replace.** Components remain compatible with normal
   FTXUI code.
2. **Keep the event loop in the application.** The library does not introduce a
   competing runtime.
3. **Separate models from views.** Streaming text and log data can be tested
   independently of terminal rendering.
4. **Keep optional features optional.** Markdown, syntax highlighting, images,
   and platform integrations should not be required by the core library.
5. **Stay domain-neutral.** The library provides UI building blocks rather than
   assumptions about a specific product.
6. **Measure large-data behavior.** Virtualization and streaming components
   should be covered by tests and benchmarks.

## Roadmap

Near-term priorities are:

- stabilize the existing component and document APIs;
- improve Unicode grapheme and terminal-cell-width handling;
- expand interaction and rendering tests;
- validate Linux, macOS, and Windows behavior;
- finish package export for optional compiled modules;
- add stronger performance benchmarks for large lists and documents;
- implement multiline editing and diff presentation;
- add terminal capability detection, clipboard support, and image backends.

The issue tracker is the source of truth for active work:

- [Open issues](https://github.com/Artem535/Terminal-UI-Kit/issues)
- [Pull requests](https://github.com/Artem535/Terminal-UI-Kit/pulls)

## Contributing

Contributions are welcome, especially in the following areas:

- bug reports with a minimal reproduction;
- rendering and interaction tests;
- Unicode and terminal compatibility;
- performance benchmarks;
- documentation and examples;
- small, focused components that fit the existing architecture.

Before opening a pull request, read
[`CONTRIBUTING.md`](CONTRIBUTING.md).

The project uses C++20, CMake as the authoritative build system, and formatting
and static-analysis rules defined by `.clang-format` and `.clang-tidy`.

## Security

Please report security issues according to
[`SECURITY.md`](SECURITY.md) rather than opening a public issue.

## License

Terminal UI Kit is available under the [MIT License](LICENSE).
