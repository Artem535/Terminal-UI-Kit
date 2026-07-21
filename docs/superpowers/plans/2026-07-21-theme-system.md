# Theme System Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the `Theme` type (struct of semantic `TextStyle` roles), two built-in themes (`default_dark_theme()`, `default_light_theme()`), and a `without_color()` no-color fallback transform, unblocking PR4 (Basic Components).

**Architecture:** A single new header/source pair (`theme/theme.h`, `theme/theme.cc`) added to the existing `terminal_ui_kit_core` CMake target — Theme has no dedicated build target, matching the pattern already documented in `src/terminal_ui_kit/CMakeLists.txt`. Built on top of the existing `core::TextStyle`/`core::Color` types; no new dependencies.

**Tech Stack:** C++20, GoogleTest, CMake (authoritative) + Xmake (header-glob parity only).

## Global Constraints

- C++20 minimum language standard (PRD §4.1 item 3).
- Public API must contain no agent-specific or domain-specific types (PRD §8.4).
- Code style: types `PascalCase`, functions/methods/locals `snake_case`, private fields `snake_case_`, constants `kPascalCase`, files `snake_case.h`/`snake_case.cc` (PRD §47).
- Core models must be testable without a physical terminal (PRD §8.6).
- Theme has no dedicated CMake or Xmake target; it is exposed through `TerminalUiKit::Core` (per existing comment in `src/terminal_ui_kit/CMakeLists.txt`).
- Spec: `docs/superpowers/specs/2026-07-21-theme-system-design.md`.

## Reference: existing types being built on

`include/terminal_ui_kit/core/text_style.h` already defines:

```cpp
namespace terminal_ui_kit {

struct Color {
  std::uint8_t red = 0;
  std::uint8_t green = 0;
  std::uint8_t blue = 0;

  friend bool operator==(const Color&, const Color&) = default;
};

struct TextStyle {
  bool bold = false;
  bool italic = false;
  bool underline = false;
  bool dim = false;
  bool strikethrough = false;
  std::optional<Color> foreground;
  std::optional<Color> background;

  friend bool operator==(const TextStyle&, const TextStyle&) = default;
};

}  // namespace terminal_ui_kit
```

Build/test commands in this plan use the already-configured `build-debug`
directory (git-ignored, `TERMINAL_UI_KIT_BUILD_TESTS=ON`). If it does not
exist in your checkout, configure it first:

```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug -DTERMINAL_UI_KIT_BUILD_TESTS=ON
```

---

### Task 1: Theme struct

**Files:**
- Create: `include/terminal_ui_kit/theme/theme.h`
- Modify: `tests/terminal_ui_kit/unit/CMakeLists.txt`
- Test: `tests/terminal_ui_kit/unit/theme_test.cc` (new file)

**Interfaces:**
- Consumes: `terminal_ui_kit::TextStyle`, `terminal_ui_kit::Color` from `terminal_ui_kit/core/text_style.h`.
- Produces: `struct terminal_ui_kit::Theme` with public fields `primary`, `secondary`, `muted`, `success`, `warning`, `error`, `accent`, `code`, `addition`, `deletion`, `border`, `selected`, `focused` (all `TextStyle`), plus `operator==`/`operator!=`. Later tasks add free functions that return/consume `Theme` by value or `const Theme&`.

- [ ] **Step 1: Write the failing test file**

Create `tests/terminal_ui_kit/unit/theme_test.cc`:

```cpp
#include "terminal_ui_kit/theme/theme.h"

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TEST(Theme, DefaultConstructedRolesAreDefaultTextStyle) {
  Theme theme;
  EXPECT_EQ(theme.primary, TextStyle{});
  EXPECT_EQ(theme.error, TextStyle{});
}

TEST(Theme, EqualityComparesAllRoles) {
  Theme a;
  Theme b;
  EXPECT_EQ(a, b);

  b.accent.bold = true;
  EXPECT_NE(a, b);
}

}  // namespace
}  // namespace terminal_ui_kit
```

- [ ] **Step 2: Register the test file**

In `tests/terminal_ui_kit/unit/CMakeLists.txt`, add `theme_test.cc` to the
`add_executable` source list:

```cmake
add_executable(terminal_ui_kit_unit_tests
  text_position_test.cc
  text_range_test.cc
  styled_text_test.cc
  text_wrap_test.cc
  theme_test.cc)
```

- [ ] **Step 3: Run to verify it fails**

Run: `cmake --build build-debug --target terminal_ui_kit_unit_tests`
Expected: FAIL — compile error, `terminal_ui_kit/theme/theme.h` not found.

- [ ] **Step 4: Write the minimal header**

Create `include/terminal_ui_kit/theme/theme.h`:

```cpp
#pragma once

#include "terminal_ui_kit/core/text_style.h"

namespace terminal_ui_kit {

// Semantic style roles shared across components (PRD section 14.1).
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

  friend bool operator==(const Theme&, const Theme&) = default;
};

}  // namespace terminal_ui_kit
```

- [ ] **Step 5: Run to verify it passes**

Run: `cmake --build build-debug --target terminal_ui_kit_unit_tests && ctest --test-dir build-debug --output-on-failure -R Theme`
Expected: PASS — 2 tests (`Theme.DefaultConstructedRolesAreDefaultTextStyle`, `Theme.EqualityComparesAllRoles`).

- [ ] **Step 6: Commit**

```bash
git add include/terminal_ui_kit/theme/theme.h tests/terminal_ui_kit/unit/theme_test.cc tests/terminal_ui_kit/unit/CMakeLists.txt
git commit -m "Add Theme struct with semantic style roles"
```

---

### Task 2: Built-in dark and light themes

**Files:**
- Modify: `include/terminal_ui_kit/theme/theme.h`
- Create: `src/terminal_ui_kit/theme/theme.cc`
- Modify: `src/terminal_ui_kit/CMakeLists.txt:38` (the `add_library(terminal_ui_kit_core core/text_wrap.cc)` line)
- Modify: `xmake.lua`
- Test: `tests/terminal_ui_kit/unit/theme_test.cc` (append tests)

**Interfaces:**
- Consumes: `Theme` struct from Task 1; `Color` from `terminal_ui_kit/core/text_style.h`.
- Produces: `const Theme& default_dark_theme();` and `const Theme& default_light_theme();`, both declared in `terminal_ui_kit/theme/theme.h`. Task 3's `without_color()` takes a `Theme` produced by these.

- [ ] **Step 1: Add failing tests**

Append to `tests/terminal_ui_kit/unit/theme_test.cc` (inside the existing
anonymous namespace, before the closing `}  // namespace`):

```cpp
TEST(Theme, DefaultDarkThemeIsStable) {
  EXPECT_EQ(default_dark_theme(), default_dark_theme());
}

TEST(Theme, DefaultLightThemeIsStable) {
  EXPECT_EQ(default_light_theme(), default_light_theme());
}

TEST(Theme, DarkAndLightThemesDiffer) {
  EXPECT_NE(default_dark_theme(), default_light_theme());
}

TEST(Theme, DarkThemeMarksNonColorAttributes) {
  const Theme& theme = default_dark_theme();
  EXPECT_TRUE(theme.error.bold);
  EXPECT_TRUE(theme.focused.bold);
  EXPECT_TRUE(theme.muted.dim);
}

TEST(Theme, LightThemeMarksNonColorAttributes) {
  const Theme& theme = default_light_theme();
  EXPECT_TRUE(theme.error.bold);
  EXPECT_TRUE(theme.focused.bold);
  EXPECT_TRUE(theme.muted.dim);
}

TEST(Theme, DarkThemeSetsRepresentativeColors) {
  const Theme& theme = default_dark_theme();
  ASSERT_TRUE(theme.primary.foreground.has_value());
  EXPECT_EQ(*theme.primary.foreground, (Color{0xe6, 0xed, 0xf3}));
  ASSERT_TRUE(theme.error.foreground.has_value());
  EXPECT_EQ(*theme.error.foreground, (Color{0xf8, 0x51, 0x49}));
  ASSERT_TRUE(theme.code.background.has_value());
  EXPECT_EQ(*theme.code.background, (Color{0x16, 0x1b, 0x22}));
}

TEST(Theme, LightThemeSetsRepresentativeColors) {
  const Theme& theme = default_light_theme();
  ASSERT_TRUE(theme.primary.foreground.has_value());
  EXPECT_EQ(*theme.primary.foreground, (Color{0x1f, 0x23, 0x28}));
  ASSERT_TRUE(theme.error.foreground.has_value());
  EXPECT_EQ(*theme.error.foreground, (Color{0xd1, 0x24, 0x2f}));
  ASSERT_TRUE(theme.code.background.has_value());
  EXPECT_EQ(*theme.code.background, (Color{0xf6, 0xf8, 0xfa}));
}
```

- [ ] **Step 2: Run to verify it fails**

Run: `cmake --build build-debug --target terminal_ui_kit_unit_tests`
Expected: FAIL — compile error, `default_dark_theme`/`default_light_theme` not declared.

- [ ] **Step 3: Declare the functions**

In `include/terminal_ui_kit/theme/theme.h`, add after the `Theme` struct
(before the closing `}  // namespace terminal_ui_kit`):

```cpp
// Built-in themes inspired by the GitHub Primer dark/light palettes (PRD
// section 14.2). Values are approximate, not verified byte-for-byte against
// Primer's published design tokens.
const Theme& default_dark_theme();
const Theme& default_light_theme();
```

- [ ] **Step 4: Implement the themes**

Create `src/terminal_ui_kit/theme/theme.cc`:

```cpp
#include "terminal_ui_kit/theme/theme.h"

#include <cstdint>

namespace terminal_ui_kit {
namespace {

constexpr Color Hex(std::uint32_t rgb) {
  return Color{
      static_cast<std::uint8_t>((rgb >> 16) & 0xFF),
      static_cast<std::uint8_t>((rgb >> 8) & 0xFF),
      static_cast<std::uint8_t>(rgb & 0xFF),
  };
}

Theme MakeDarkTheme() {
  Theme theme;

  theme.primary.foreground = Hex(0xe6edf3);
  theme.secondary.foreground = Hex(0x848d97);
  theme.muted.foreground = Hex(0x6e7681);
  theme.muted.dim = true;
  theme.success.foreground = Hex(0x3fb950);
  theme.warning.foreground = Hex(0xd29922);
  theme.error.foreground = Hex(0xf85149);
  theme.error.bold = true;
  theme.accent.foreground = Hex(0x58a6ff);
  theme.code.foreground = Hex(0xe6edf3);
  theme.code.background = Hex(0x161b22);
  theme.addition.foreground = Hex(0x3fb950);
  theme.addition.background = Hex(0x0f2c1e);
  theme.deletion.foreground = Hex(0xf85149);
  theme.deletion.background = Hex(0x331418);
  theme.border.foreground = Hex(0x30363d);
  theme.selected.background = Hex(0x142f60);
  theme.focused.foreground = Hex(0x58a6ff);
  theme.focused.bold = true;

  return theme;
}

Theme MakeLightTheme() {
  Theme theme;

  theme.primary.foreground = Hex(0x1f2328);
  theme.secondary.foreground = Hex(0x656d76);
  theme.muted.foreground = Hex(0x8c959f);
  theme.muted.dim = true;
  theme.success.foreground = Hex(0x1a7f37);
  theme.warning.foreground = Hex(0x9a6700);
  theme.error.foreground = Hex(0xd1242f);
  theme.error.bold = true;
  theme.accent.foreground = Hex(0x0969da);
  theme.code.foreground = Hex(0x1f2328);
  theme.code.background = Hex(0xf6f8fa);
  theme.addition.foreground = Hex(0x1a7f37);
  theme.addition.background = Hex(0xe6ffec);
  theme.deletion.foreground = Hex(0xd1242f);
  theme.deletion.background = Hex(0xffebe9);
  theme.border.foreground = Hex(0xd1d9e0);
  theme.selected.background = Hex(0xddf4ff);
  theme.focused.foreground = Hex(0x0969da);
  theme.focused.bold = true;

  return theme;
}

}  // namespace

const Theme& default_dark_theme() {
  static const Theme theme = MakeDarkTheme();
  return theme;
}

const Theme& default_light_theme() {
  static const Theme theme = MakeLightTheme();
  return theme;
}

}  // namespace terminal_ui_kit
```

- [ ] **Step 5: Wire the CMake source into the core target**

In `src/terminal_ui_kit/CMakeLists.txt`, change:

```cmake
add_library(terminal_ui_kit_core core/text_wrap.cc)
```

to:

```cmake
add_library(terminal_ui_kit_core core/text_wrap.cc theme/theme.cc)
```

- [ ] **Step 6: Add Xmake header-glob parity**

In `xmake.lua`, immediately after the `for _, name in ipairs(...)` loop
that defines module targets (after its `end`), add:

```lua
-- Theme (include/terminal_ui_kit/theme/) has no dedicated target, same as
-- the CMake side (src/terminal_ui_kit/CMakeLists.txt) -- it is exposed
-- through terminal_ui_kit_core.
target("terminal_ui_kit_core")
    add_headerfiles("include/terminal_ui_kit/theme/**.h")
target_end()
```

- [ ] **Step 7: Run to verify it passes**

Run: `cmake --build build-debug --target terminal_ui_kit_unit_tests && ctest --test-dir build-debug --output-on-failure -R Theme`
Expected: PASS — 8 tests total for the `Theme` suite so far.

- [ ] **Step 8: Commit**

```bash
git add include/terminal_ui_kit/theme/theme.h src/terminal_ui_kit/theme/theme.cc src/terminal_ui_kit/CMakeLists.txt xmake.lua tests/terminal_ui_kit/unit/theme_test.cc
git commit -m "Add default dark and light themes"
```

---

### Task 3: `without_color()` fallback

**Files:**
- Modify: `include/terminal_ui_kit/theme/theme.h`
- Modify: `src/terminal_ui_kit/theme/theme.cc`
- Test: `tests/terminal_ui_kit/unit/theme_test.cc` (append tests)

**Interfaces:**
- Consumes: `Theme` (from Tasks 1-2), `TextStyle` (from `core/text_style.h`).
- Produces: `Theme without_color(const Theme& theme);` — final public surface of this plan.

- [ ] **Step 1: Add failing tests**

Append to `tests/terminal_ui_kit/unit/theme_test.cc`:

```cpp
TEST(Theme, WithoutColorStripsForegroundAndBackgroundFromEveryRole) {
  const Theme stripped = without_color(default_dark_theme());

  const TextStyle* roles[] = {
      &stripped.primary,  &stripped.secondary, &stripped.muted,
      &stripped.success,  &stripped.warning,   &stripped.error,
      &stripped.accent,   &stripped.code,      &stripped.addition,
      &stripped.deletion, &stripped.border,    &stripped.selected,
      &stripped.focused,
  };

  for (const TextStyle* style : roles) {
    EXPECT_FALSE(style->foreground.has_value());
    EXPECT_FALSE(style->background.has_value());
  }
}

TEST(Theme, WithoutColorPreservesNonColorAttributes) {
  const Theme stripped = without_color(default_dark_theme());

  EXPECT_TRUE(stripped.error.bold);
  EXPECT_TRUE(stripped.focused.bold);
  EXPECT_TRUE(stripped.muted.dim);
}
```

- [ ] **Step 2: Run to verify it fails**

Run: `cmake --build build-debug --target terminal_ui_kit_unit_tests`
Expected: FAIL — compile error, `without_color` not declared.

- [ ] **Step 3: Declare the function**

In `include/terminal_ui_kit/theme/theme.h`, add after the built-in theme
declarations (before the closing `}  // namespace terminal_ui_kit`):

```cpp
// Strips foreground/background from every role, keeping bold/italic/
// underline/dim/strikethrough untouched. Consumers that need visual
// distinction without color (status icons, diff +/- markers) already carry
// that meaning outside of color.
Theme without_color(const Theme& theme);
```

- [ ] **Step 4: Implement the function**

In `src/terminal_ui_kit/theme/theme.cc`, add at the end, before the closing
`}  // namespace terminal_ui_kit`:

```cpp
Theme without_color(const Theme& theme) {
  Theme result = theme;

  TextStyle* roles[] = {
      &result.primary,  &result.secondary, &result.muted,
      &result.success,  &result.warning,   &result.error,
      &result.accent,   &result.code,      &result.addition,
      &result.deletion, &result.border,    &result.selected,
      &result.focused,
  };

  for (TextStyle* style : roles) {
    style->foreground.reset();
    style->background.reset();
  }

  return result;
}
```

- [ ] **Step 5: Run to verify it passes**

Run: `cmake --build build-debug --target terminal_ui_kit_unit_tests && ctest --test-dir build-debug --output-on-failure -R Theme`
Expected: PASS — 10 tests total for the `Theme` suite.

- [ ] **Step 6: Run the full test suite to check for regressions**

Run: `cmake --build build-debug && ctest --test-dir build-debug --output-on-failure`
Expected: PASS — all existing suites (`TextPosition`, `TextRange`, `StyledText`, `TextWrap`, rendering tests) plus the new `Theme` suite, all green.

- [ ] **Step 7: Commit**

```bash
git add include/terminal_ui_kit/theme/theme.h src/terminal_ui_kit/theme/theme.cc tests/terminal_ui_kit/unit/theme_test.cc
git commit -m "Add without_color no-color fallback transform"
```

---

## Plan Self-Review Notes

- **Spec coverage:** `Theme` struct (Task 1), `default_dark_theme()` /
  `default_light_theme()` with the full palette table and non-color
  attributes (Task 2), `without_color()` (Task 3), CMake wiring (Task 2 step
  5), Xmake header parity (Task 2 step 6), user overrides (no task needed —
  spec states this requires no dedicated API, just struct copy/modify by
  the caller). All spec sections are covered.
- **Placeholder scan:** none found — every step has complete code.
- **Type consistency:** `Theme`, `TextStyle`, `Color`, `default_dark_theme`,
  `default_light_theme`, `without_color` are named identically across all
  three tasks.
