# theme_viewer Example App Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add `examples/theme_viewer`, an interactive FTXUI app that demonstrates the Theme system (13 semantic roles, dark/light themes, `without_color()`) — the project's first example app and first examples build wiring.

**Architecture:** A single `main.cc` using standard FTXUI idiom (`ScreenInteractive::Fullscreen()` + `Renderer` + `CatchEvent` + `Loop`), plus the CMake wiring needed to build it (`examples/CMakeLists.txt` is new — no example has ever built in this repo before).

**Tech Stack:** C++20, FTXUI (`ftxui::screen`/`dom`/`component`, already fetched via `TERMINAL_UI_KIT_BUILD_EXAMPLES`), CMake only (no Xmake wiring — see Global Constraints).

## Global Constraints

- C++20 minimum language standard.
- Code style: types PascalCase, functions/methods/locals snake_case, files snake_case.h/snake_case.cc.
- No new library API: the `TextStyle` → FTXUI `Decorator` conversion is local to this example's `main.cc`, not part of `terminal_ui_kit`'s public headers.
- CMake only — do not add Xmake wiring for this example (Xmake currently builds neither tests nor examples anywhere in this repo; extending that gap here is an explicit, approved scope decision, not an oversight).
- No automated test/CI check of this example's rendered output (approved non-goal — Theme's actual behavior is already covered by `tests/terminal_ui_kit/unit/theme_test.cc`'s 11 tests). Verification for this plan is: configure succeeds, the target builds cleanly, and the full existing test suite still passes unaffected.
- Spec: `docs/superpowers/specs/2026-07-21-theme-viewer-example-design.md`.

## Reference: Theme API being demonstrated

`include/terminal_ui_kit/theme/theme.h` (already on this branch) declares:

```cpp
namespace terminal_ui_kit {

struct Theme {
  TextStyle primary;
  TextStyle secondary;
  TextStyle muted;
  TextStyle success;
  TextStyle warning;
  TextStyle error;
  TextStyle accent;
  TextStyle code;
  TextStyle addition;
  TextStyle deletion;
  TextStyle border;
  TextStyle selected;
  TextStyle focused;
};

const Theme& default_dark_theme();
const Theme& default_light_theme();
Theme without_color(const Theme& theme);

}  // namespace terminal_ui_kit
```

`include/terminal_ui_kit/core/text_style.h` (already in the repo) declares:

```cpp
namespace terminal_ui_kit {

struct Color {
  std::uint8_t red = 0;
  std::uint8_t green = 0;
  std::uint8_t blue = 0;
};

struct TextStyle {
  bool bold = false;
  bool italic = false;
  bool underline = false;
  bool dim = false;
  bool strikethrough = false;
  std::optional<Color> foreground;
  std::optional<Color> background;
};

}  // namespace terminal_ui_kit
```

Relevant FTXUI symbols this task uses (all in `<ftxui/dom/elements.hpp>` /
`<ftxui/component/...>`, already vendored under `build-debug/_deps/ftxui-src`):
`Element`, `Elements`, `Decorator` (`using Decorator = std::function<Element(Element)>`),
`nothing(Element)`, `bold(Element)`, `dim(Element)`, `underlined(Element)`,
`strikethrough(Element)`, `color(Color)` / `bgcolor(Color)` (both return
`Decorator`), `ftxui::Color::RGB(uint8_t, uint8_t, uint8_t)`, `text(std::string)`,
`hbox(Elements)`, `vbox(Elements)`, `separator()`, `border` (a `Decorator`),
`size(WidthOrHeight, Constraint, int)` with enums `WIDTH`/`HEIGHT` and
`LESS_THAN`/`EQUAL`/`GREATER_THAN`, `Renderer(std::function<Element()>)`,
`CatchEvent(std::function<bool(Event)>)`, `Event::Character(std::string)`,
`Event::Escape`, `ScreenInteractive::Fullscreen()`, `screen.Loop(component)`,
`screen.ExitLoopClosure()`.

---

### Task 1: `theme_viewer` example app

**Files:**
- Create: `examples/theme_viewer/main.cc`
- Create: `examples/theme_viewer/CMakeLists.txt`
- Create: `examples/CMakeLists.txt`

**Interfaces:**
- Consumes: `terminal_ui_kit::Theme`, `terminal_ui_kit::TextStyle`,
  `terminal_ui_kit::Color`, `terminal_ui_kit::default_dark_theme()`,
  `terminal_ui_kit::default_light_theme()`, `terminal_ui_kit::without_color()`
  (all from `terminal_ui_kit/theme/theme.h`), plus FTXUI listed above.
- Produces: executable target `terminal_ui_kit_example_theme_viewer`. Nothing
  else in the plan depends on this task — it is the final artifact.

This task has no unit tests (see Global Constraints — automated output
testing is an approved non-goal for this example), so there is no RED/GREEN
TDD cycle. Verification is: clean configure, clean build, and no
regressions in the existing 27-test suite.

- [ ] **Step 1: Write `examples/theme_viewer/main.cc`**

```cpp
#include <string>
#include <vector>

#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include "terminal_ui_kit/theme/theme.h"

namespace {

using terminal_ui_kit::Color;
using terminal_ui_kit::TextStyle;
using terminal_ui_kit::Theme;

// FTXUI has no `italic` decorator (only bold/dim/underlined/
// underlinedDouble/blink/strikethrough/color/bgcolor), so TextStyle::italic
// has nothing to map to here; no built-in theme role sets it today, so this
// has no visible effect.
ftxui::Decorator style_to_decorator(const TextStyle& style) {
  ftxui::Decorator decorator = ftxui::nothing;

  if (style.bold) {
    decorator = decorator | ftxui::Decorator(ftxui::bold);
  }
  if (style.dim) {
    decorator = decorator | ftxui::Decorator(ftxui::dim);
  }
  if (style.underline) {
    decorator = decorator | ftxui::Decorator(ftxui::underlined);
  }
  if (style.strikethrough) {
    decorator = decorator | ftxui::Decorator(ftxui::strikethrough);
  }
  if (style.foreground) {
    const Color& fg = *style.foreground;
    decorator = decorator | ftxui::color(ftxui::Color::RGB(fg.red, fg.green, fg.blue));
  }
  if (style.background) {
    const Color& bg = *style.background;
    decorator = decorator | ftxui::bgcolor(ftxui::Color::RGB(bg.red, bg.green, bg.blue));
  }

  return decorator;
}

struct RoleDemo {
  std::string name;
  TextStyle Theme::*field;
  std::string sample;
};

const std::vector<RoleDemo>& roles() {
  static const std::vector<RoleDemo> demo = {
      {"primary", &Theme::primary, "Sample text"},
      {"secondary", &Theme::secondary, "Sample text"},
      {"muted", &Theme::muted, "Sample text"},
      {"success", &Theme::success, "Sample text"},
      {"warning", &Theme::warning, "Sample text"},
      {"error", &Theme::error, "Sample text"},
      {"accent", &Theme::accent, "Sample text"},
      {"code", &Theme::code, "Sample text"},
      {"addition", &Theme::addition, "+ added line"},
      {"deletion", &Theme::deletion, "- removed line"},
      {"border", &Theme::border, "Sample text"},
      {"selected", &Theme::selected, "Sample text"},
      {"focused", &Theme::focused, "Sample text"},
  };
  return demo;
}

}  // namespace

int main() {
  bool dark = true;
  bool color_on = true;

  auto screen = ftxui::ScreenInteractive::Fullscreen();

  auto renderer = ftxui::Renderer([&] {
    Theme base = dark ? terminal_ui_kit::default_dark_theme()
                       : terminal_ui_kit::default_light_theme();
    Theme theme = color_on ? base : terminal_ui_kit::without_color(base);

    ftxui::Elements rows;
    for (const auto& role : roles()) {
      const TextStyle& style = theme.*(role.field);
      rows.push_back(ftxui::hbox({
          ftxui::text(role.name) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12),
          ftxui::text(role.sample) | style_to_decorator(style),
      }));
    }

    std::string status = std::string("Theme: ") + (dark ? "Dark" : "Light") +
                          " | Color: " + (color_on ? "On" : "Off");

    return ftxui::vbox({
               ftxui::text("Terminal UI Kit - Theme Viewer") | ftxui::bold,
               ftxui::text("[t] toggle dark/light   [c] toggle color   [q] quit"),
               ftxui::separator(),
               ftxui::vbox(std::move(rows)),
               ftxui::separator(),
               ftxui::text(status),
           }) |
           ftxui::border;
  });

  renderer |= ftxui::CatchEvent([&](ftxui::Event event) {
    if (event == ftxui::Event::Character("t")) {
      dark = !dark;
      return true;
    }
    if (event == ftxui::Event::Character("c")) {
      color_on = !color_on;
      return true;
    }
    if (event == ftxui::Event::Character("q") || event == ftxui::Event::Escape) {
      screen.ExitLoopClosure()();
      return true;
    }
    return false;
  });

  screen.Loop(renderer);
  return 0;
}
```

- [ ] **Step 2: Write `examples/theme_viewer/CMakeLists.txt`**

```cmake
add_executable(terminal_ui_kit_example_theme_viewer main.cc)

target_link_libraries(terminal_ui_kit_example_theme_viewer PRIVATE
  TerminalUiKit::Core
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_terminal_ui_kit_warnings(terminal_ui_kit_example_theme_viewer)
```

- [ ] **Step 3: Write `examples/CMakeLists.txt`**

```cmake
# Example applications demonstrating library components in isolation (PRD
# section 54). Each subdirectory is a standalone add_executable target; add
# a new add_subdirectory(...) line here as each future task adds its own
# example.

add_subdirectory(theme_viewer)
```

- [ ] **Step 4: Configure with examples enabled**

Run: `cmake -S . -B build-debug -DTERMINAL_UI_KIT_BUILD_EXAMPLES=ON`
Expected: reconfigures the existing `build-debug` cache (keeps
`TERMINAL_UI_KIT_BUILD_TESTS=ON` already set), ends with `-- Generating
done` and no error. You should NOT see the "examples has no CMakeLists.txt
yet" message from this point on.

- [ ] **Step 5: Build the example target**

Run: `cmake --build build-debug --target terminal_ui_kit_example_theme_viewer`
Expected: builds cleanly, ends with `[100%] Built target
terminal_ui_kit_example_theme_viewer`, no warnings (the target has
`target_terminal_ui_kit_warnings` applied, same as every other compiled
target in this repo).

- [ ] **Step 6: Confirm no regressions in the existing test suite**

Run: `cmake --build build-debug && ctest --test-dir build-debug --output-on-failure`
Expected: PASS — all 27 existing tests still passing (this task adds a new
independent executable target; it must not change anything under `src/`,
`include/`, or `tests/`).

- [ ] **Step 7: Commit**

```bash
git add examples/CMakeLists.txt examples/theme_viewer/
git commit -m "Add theme_viewer example app"
```

---

## Plan Self-Review Notes

- **Spec coverage:** interactive dark/light + color/no-color toggling
  (Step 1), CMake-only wiring (Steps 2-3), no library API added (Step 1's
  `style_to_decorator` stays local to `main.cc`), no Xmake changes (none
  made). All spec sections covered.
- **Placeholder scan:** none found — full code given for every file.
- **Type consistency:** `Theme`, `TextStyle`, `Color`,
  `default_dark_theme`, `default_light_theme`, `without_color` match their
  declarations in `include/terminal_ui_kit/theme/theme.h` exactly (already
  committed on this branch, not re-declared here).
