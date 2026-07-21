# VirtualList Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a fixed-height, viewport-virtualized `VirtualList` and
`VirtualListModel`, with keyboard/mouse navigation and a 100,000-row example.

**Architecture:** `VirtualListImpl` is a focusable `ftxui::ComponentBase`
owned by `VirtualListModel`. It measures its allocated FTXUI box with
`reflect()`, computes a fixed-height visible index range, and invokes the row
renderer only for that range. Variable-height layout, caching, overscan, and
follow-end remain outside this PR.

**Tech Stack:** C++20, FTXUI DOM/component/screen, GoogleTest, CMake.

## Global Constraints

- Types use `PascalCase`; functions, methods, and locals use `snake_case`.
- Private data members use `snake_case_`; files use `snake_case.h` and
  `snake_case.cc`.
- Public APIs contain no application/domain-specific types.
- Tests use `test_support::render_to_screen` / `render_to_text` and public
  `ComponentBase` methods; they do not create `ScreenInteractive`.
- The list renders exactly the fixed-height viewport range, never all items.
- `item_height <= 0` becomes one; empty lists have no selection; a non-empty
  list starts selected at index zero without calling `on_select`.
- `scroll_to_index()` never changes selection; `select_index()` reveals the
  item and calls `on_select` only when selection changes.
- No Xmake source/example wiring is added, following the existing examples.

---

### Task 1: Public API, empty-list behaviour, and CMake wiring

**Files:**
- Create: `include/terminal_ui_kit/components/virtual_list.h`
- Create: `src/terminal_ui_kit/components/virtual_list.cc`
- Modify: `src/terminal_ui_kit/CMakeLists.txt`
- Modify: `tests/terminal_ui_kit/rendering/CMakeLists.txt`
- Create: `tests/terminal_ui_kit/rendering/virtual_list_test.cc`

**Interfaces:**
- Produces `VirtualListOptions`, `VirtualListModel`, and `VirtualList` in
  `terminal_ui_kit`.
- Consumes FTXUI `Element` and `Component` only; this component has no Theme
  dependency.

- [ ] **Step 1: Write the failing public-API tests**

Create `tests/terminal_ui_kit/rendering/virtual_list_test.cc`:

```cpp
#include "terminal_ui_kit/components/virtual_list.h"

#include <string>
#include <utility>

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TEST(VirtualListModel, EmptyListHasNoSelection) {
  VirtualListOptions options;
  options.item_count = [] { return std::size_t{0}; };
  options.render_item = [](std::size_t, int) { return ftxui::text("unused"); };

  VirtualListModel model(std::move(options));

  EXPECT_EQ(model.selected_index(), std::nullopt);
  EXPECT_FALSE(model.component()->Focusable());
}

TEST(VirtualListModel, NonEmptyListInitiallySelectsFirstItemWithoutCallback) {
  std::size_t callback_count = 0;
  VirtualListOptions options;
  options.item_count = [] { return std::size_t{3}; };
  options.render_item = [](std::size_t index, int) {
    return ftxui::text(std::to_string(index));
  };
  options.on_select = [&] { ++callback_count; };

  VirtualListModel model(std::move(options));

  EXPECT_EQ(model.selected_index(), std::optional<std::size_t>{0});
  EXPECT_EQ(callback_count, 0U);
  EXPECT_TRUE(model.component()->Focusable());
}

}  // namespace
}  // namespace terminal_ui_kit
```

Add `virtual_list_test.cc` to the rendering test executable and add
`components/virtual_list.cc` to `terminal_ui_kit_components`.

- [ ] **Step 2: Verify the test fails**

Run:

```bash
cmake --build build-debug --target terminal_ui_kit_rendering_tests
```

Expected: the build fails because
`terminal_ui_kit/components/virtual_list.h` does not exist.

- [ ] **Step 3: Add the public header and a minimal component implementation**

Create `include/terminal_ui_kit/components/virtual_list.h`:

```cpp
#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

namespace terminal_ui_kit {

struct VirtualListOptions {
  std::function<std::size_t()> item_count;
  std::function<ftxui::Element(std::size_t index, int width)> render_item;
  int item_height = 1;
  std::function<void(std::size_t index)> on_select;
};

class VirtualListImpl;

class VirtualListModel {
 public:
  explicit VirtualListModel(VirtualListOptions options);

  ftxui::Component component() const;
  void scroll_to_index(std::size_t index);
  void select_index(std::size_t index);
  std::optional<std::size_t> selected_index() const;

 private:
  std::shared_ptr<VirtualListImpl> impl_;
};

ftxui::Component VirtualList(VirtualListOptions options);

}  // namespace terminal_ui_kit
```

Create `src/terminal_ui_kit/components/virtual_list.cc` with the minimal
state normalization needed for the two tests. `VirtualListImpl` must inherit
`ftxui::ComponentBase`, initialize `selected_index_` to zero only when
`item_count()` is non-zero, render an empty element, and report focusability
from whether a selection exists:

```cpp
class VirtualListImpl : public ftxui::ComponentBase {
 public:
  explicit VirtualListImpl(VirtualListOptions options) : options_(std::move(options)) {
    Normalize();
  }

  void ScrollToIndex(std::size_t) {}
  void SelectIndex(std::size_t) {}
  std::optional<std::size_t> SelectedIndex() const { return selected_index_; }

 private:
  ftxui::Element Render() override { return ftxui::text(""); }
  bool Focusable() const override { return selected_index_.has_value(); }
  void Normalize() {
    if (!options_.item_count || options_.item_count() == 0) {
      selected_index_.reset();
      return;
    }
    selected_index_ = 0;
  }

  VirtualListOptions options_;
  std::optional<std::size_t> selected_index_;
};
```

Define the model methods as direct forwarding methods and make
`VirtualList(options)` return `ftxui::Make<VirtualListImpl>(std::move(options))`.

- [ ] **Step 4: Verify the API tests pass**

Run:

```bash
cmake --build build-debug --target terminal_ui_kit_rendering_tests
ctest --test-dir build-debug --output-on-failure -R VirtualListModel
```

Expected: both `VirtualListModel` tests pass.

- [ ] **Step 5: Commit the wiring and public API**

```bash
git add include/terminal_ui_kit/components/virtual_list.h \
  src/terminal_ui_kit/components/virtual_list.cc \
  src/terminal_ui_kit/CMakeLists.txt \
  tests/terminal_ui_kit/rendering/CMakeLists.txt \
  tests/terminal_ui_kit/rendering/virtual_list_test.cc
git commit -m "Add VirtualList public API"
```

### Task 2: Virtualized rendering, navigation, and model control

**Files:**
- Modify: `src/terminal_ui_kit/components/virtual_list.cc`
- Modify: `tests/terminal_ui_kit/rendering/virtual_list_test.cc`

**Interfaces:**
- Consumes the API from Task 1, FTXUI `Event`, `Mouse`, `reflect`, `size`,
  `yflex`, and `RequestAnimationFrame`.
- Produces a fixed-height virtual viewport, keyboard and wheel interaction,
  `scroll_to_index`, `select_index`, and `on_select` semantics.

- [ ] **Step 1: Add failing rendering and interaction tests**

Append these representative tests (and the necessary `<string>`,
`<vector>`, FTXUI event/mouse, and virtual-screen includes):

```cpp
TEST(VirtualList, RendersOnlyViewportItemsFromLargeList) {
  std::size_t render_count = 0;
  VirtualListOptions options;
  options.item_count = [] { return std::size_t{100000}; };
  options.render_item = [&render_count](std::size_t index, int) {
    ++render_count;
    return ftxui::text(std::to_string(index));
  };

  VirtualListModel model(std::move(options));
  test_support::render_to_screen(model.component()->Render(), 20, 4);
  render_count = 0;
  test_support::render_to_screen(model.component()->Render(), 20, 4);

  EXPECT_LE(render_count, 4U);
}

TEST(VirtualList, ArrowDownChangesSelectionAndInvokesCallback) {
  std::vector<std::size_t> selections;
  VirtualListOptions options;
  options.item_count = [] { return std::size_t{3}; };
  options.render_item = [](std::size_t index, int) {
    return ftxui::text(std::to_string(index));
  };
  options.on_select = [&selections](std::size_t index) { selections.push_back(index); };
  VirtualListModel model(std::move(options));

  EXPECT_TRUE(model.component()->OnEvent(ftxui::Event::ArrowDown));

  EXPECT_EQ(model.selected_index(), std::optional<std::size_t>{1});
  EXPECT_EQ(selections, std::vector<std::size_t>{1});
}

TEST(VirtualList, SelectedItemIsInverted) {
  VirtualListOptions options;
  options.item_count = [] { return std::size_t{2}; };
  options.render_item = [](std::size_t index, int) {
    return ftxui::text(index == 0 ? "first" : "second");
  };
  VirtualListModel model(std::move(options));
  test_support::render_to_screen(model.component()->Render(), 20, 2);
  model.select_index(1);

  ftxui::Screen screen = test_support::render_to_screen(model.component()->Render(), 20, 2);
  EXPECT_FALSE(screen.PixelAt(0, 0).inverted);
  EXPECT_TRUE(screen.PixelAt(0, 1).inverted);
}

TEST(VirtualListModel, ModelControlClampsAndPreservesCallbackRules) {
  std::size_t count = 8;
  std::vector<std::size_t> selections;
  VirtualListOptions options;
  options.item_count = [&count] { return count; };
  options.render_item = [](std::size_t index, int) {
    return ftxui::text(std::to_string(index));
  };
  options.on_select = [&selections](std::size_t index) { selections.push_back(index); };
  VirtualListModel model(std::move(options));

  model.select_index(99);
  model.scroll_to_index(99);
  count = 3;
  test_support::render_to_screen(model.component()->Render(), 20, 2);

  EXPECT_EQ(model.selected_index(), std::optional<std::size_t>{2});
  EXPECT_EQ(selections, std::vector<std::size_t>{7});
}
```

Add `#include "terminal_ui_kit/testing/virtual_screen.h"` and two further
tests in the same file:

- Build an `ftxui::Mouse` with `x = 0`, `y = 0`, `button =
  ftxui::Mouse::WheelDown`, pass it through `ftxui::Event::Mouse("", mouse)`,
  and assert that the rendered first index moves down by three without
  changing `selected_index()`.
- Render once at width 20 and once at width 40, then assert that the last
  width received by `render_item` is 40. This verifies the reflected viewport
  is invalidated after resize.

- [ ] **Step 2: Verify the new tests fail**

Run:

```bash
cmake --build build-debug --target terminal_ui_kit_rendering_tests
ctest --test-dir build-debug --output-on-failure -R VirtualList
```

Expected: the viewport, interaction, inversion, wheel, and resize assertions
fail against Task 1's minimal implementation.

- [ ] **Step 3: Replace the stub with the virtualized component**

Implement these helpers inside `VirtualListImpl`:

```cpp
std::size_t item_count() const {
  return options_.item_count ? options_.item_count() : 0;
}

int item_height() const { return std::max(1, options_.item_height); }

std::size_t viewport_rows() const {
  const int height = std::max(1, box_.y_max - box_.y_min + 1);
  return static_cast<std::size_t>((height + item_height() - 1) / item_height());
}

void normalize() {
  const std::size_t count = item_count();
  if (count == 0) {
    scroll_index_ = 0;
    selected_index_.reset();
    return;
  }
  if (!selected_index_) {
    selected_index_ = 0;
  }
  *selected_index_ = std::min(*selected_index_, count - 1);
  const std::size_t max_scroll = count > viewport_rows() ? count - viewport_rows() : 0;
  scroll_index_ = std::min(scroll_index_, max_scroll);
}
```

`Render()` must call `normalize()`, render indexes in
`[scroll_index_, min(item_count(), scroll_index_ + viewport_rows()))`, force
each element to `ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, item_height())`,
and apply `ftxui::inverted` only to the selected index:

```cpp
ftxui::Element Render() override {
  normalize();
  const int width = std::max(1, box_.x_max - box_.x_min + 1);
  ftxui::Elements rows;
  const std::size_t end = std::min(item_count(), scroll_index_ + viewport_rows());
  for (std::size_t index = scroll_index_; index < end; ++index) {
    ftxui::Element row = options_.render_item(index, width) |
                         ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, item_height());
    if (selected_index_ && *selected_index_ == index) {
      row = row | ftxui::inverted;
    }
    rows.push_back(std::move(row));
  }
  return ftxui::vbox(std::move(rows)) | ftxui::yflex | ftxui::reflect(box_);
}
```

Before returning from `Render()`, compare the current reflected dimensions to
the dimensions used for this frame. If they differ, call
`ftxui::animation::RequestAnimationFrame()` so the next frame uses the new
size.

Implement `ensure_visible(index)` by moving `scroll_index_` up to `index` or
down to `index - viewport_rows() + 1`. Implement `set_selected(index)` to
change selection, reveal it, and call `on_select` only if the stored index
changed. Use it for Up, Down, PageUp, PageDown, Home, End, and
`SelectIndex`. `ScrollToIndex` only assigns a clamped `scroll_index_`.

For mouse input, reject events outside `box_`; on `WheelUp` or `WheelDown`,
subtract or add three from `scroll_index_`, clamp it, and return whether it
changed. Keep selection unchanged. Return false for unrecognized or
boundary events.

- [ ] **Step 4: Verify all VirtualList tests pass**

Run:

```bash
cmake --build build-debug --target terminal_ui_kit_rendering_tests
ctest --test-dir build-debug --output-on-failure -R VirtualList
```

Expected: all `VirtualList` and `VirtualListModel` tests pass, including the
100,000-item renderer-count regression.

- [ ] **Step 5: Format, run the complete test suite, and commit**

```bash
clang-format -i include/terminal_ui_kit/components/virtual_list.h \
  src/terminal_ui_kit/components/virtual_list.cc \
  tests/terminal_ui_kit/rendering/virtual_list_test.cc
cmake --build build-debug
ctest --test-dir build-debug --output-on-failure
git diff --check
git add include/terminal_ui_kit/components/virtual_list.h \
  src/terminal_ui_kit/components/virtual_list.cc \
  tests/terminal_ui_kit/rendering/virtual_list_test.cc
git commit -m "Add virtualized fixed-height list"
```

Expected: the entire suite passes and the diff check is clean.

### Task 3: 100,000-row VirtualList viewer

**Files:**
- Create: `examples/virtual_list_viewer/main.cc`
- Create: `examples/virtual_list_viewer/CMakeLists.txt`
- Modify: `examples/CMakeLists.txt`

**Interfaces:**
- Consumes `VirtualListModel`, `VirtualListOptions`, and `KeyHintBar`.
- Produces target `terminal_ui_kit_example_virtual_list_viewer`.

- [ ] **Step 1: Write the viewer source**

Create `examples/virtual_list_viewer/main.cc`. It must build this exact
shape: a static `kItemCount = 100000`, one `VirtualListModel`, a label-only
`render_item` callback that increments `rendered_this_frame`, and an outer
renderer that resets the counter before calling `list->Render()`:

```cpp
#include <cstddef>
#include <string>
#include <utility>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/key_hint_bar.h"
#include "terminal_ui_kit/components/virtual_list.h"
#include "terminal_ui_kit/theme/theme.h"

int main() {
  using namespace terminal_ui_kit;
  constexpr std::size_t kItemCount = 100000;
  std::size_t rendered_this_frame = 0;
  VirtualListOptions options;
  options.item_count = [] { return kItemCount; };
  options.render_item = [&rendered_this_frame](std::size_t index, int) {
    ++rendered_this_frame;
    return ftxui::text("Row " + std::to_string(index));
  };
  VirtualListModel model(std::move(options));
  ftxui::Component list = model.component();

  auto screen = ftxui::ScreenInteractive::Fullscreen();
  ftxui::Component root = ftxui::Renderer(list, [&] {
    rendered_this_frame = 0;
    ftxui::Element list_element = list->Render();
    return ftxui::vbox({
               ftxui::text("Terminal UI Kit - VirtualList Viewer") | ftxui::bold,
               ftxui::text("100,000 fixed-height rows; only the viewport is rendered.") |
                   ftxui::dim,
               ftxui::separator(),
               ftxui::text("Virtual window") | ftxui::bold,
               list_element | ftxui::flex,
               ftxui::separator(),
               ftxui::text("Rendered this frame: " +
                           std::to_string(rendered_this_frame)) |
                   ftxui::dim,
               KeyHintBar({{"up/down", "select"},
                           {"pgup/pgdn", "page"},
                           {"home/end", "jump"},
                           {"q", "quit"}},
                          default_dark_theme()),
           }) |
           ftxui::border;
  });
  root |= ftxui::CatchEvent([&](ftxui::Event event) {
    if (event == ftxui::Event::Character('q')) {
      screen.ExitLoopClosure()();
      return true;
    }
    return false;
  });
  screen.Loop(root);
}
```

- [ ] **Step 2: Add the CMake target and register the example**

Create `examples/virtual_list_viewer/CMakeLists.txt`:

```cmake
add_executable(terminal_ui_kit_example_virtual_list_viewer main.cc)

target_link_libraries(terminal_ui_kit_example_virtual_list_viewer PRIVATE
  TerminalUiKit::Components
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_terminal_ui_kit_warnings(terminal_ui_kit_example_virtual_list_viewer)
```

Append this exact line to `examples/CMakeLists.txt`:

```cmake
add_subdirectory(virtual_list_viewer)
```

- [ ] **Step 3: Build and smoke-test the viewer**

Run:

```bash
cmake -S . -B build-debug -DTERMINAL_UI_KIT_BUILD_EXAMPLES=ON
cmake --build build-debug --target terminal_ui_kit_example_virtual_list_viewer
./build-debug/examples/virtual_list_viewer/terminal_ui_kit_example_virtual_list_viewer
```

Expected: the viewer starts with a visibly inverted first row; Up/Down,
PageUp/PageDown, Home/End, wheel scroll, and `q` work. The rendered-row
counter remains near the terminal height, not 100,000.

- [ ] **Step 4: Run final verification and commit**

```bash
cmake --build build-debug
ctest --test-dir build-debug --output-on-failure
git diff --check
git add examples/virtual_list_viewer examples/CMakeLists.txt
git commit -m "Add VirtualList viewer example"
```

Expected: all targets build and all tests pass.

## Plan Self-Review

- **Spec coverage:** Tasks 1 and 2 provide the public API, fixed-height
  virtualization, normalized empty state, navigation, mouse scrolling,
  selection retention, resize invalidation, and the 100,000-item complexity
  regression. Task 3 provides the required interactive example.
- **Scope:** No task introduces variable-height cache, overscan, follow-end,
  filtering, or streaming behaviour.
- **Type consistency:** The header's `VirtualListOptions`, `VirtualListModel`,
  `VirtualList`, `scroll_to_index`, `select_index`, and `selected_index` are
  named consistently in every task.
- **Placeholder scan:** No incomplete markers or deferred implementation
  instructions remain; every code-edit task has paths, exact APIs, test
  commands, and expected outcomes.
