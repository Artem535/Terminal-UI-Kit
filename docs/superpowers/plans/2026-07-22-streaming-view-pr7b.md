# PR7B LogView + Streaming Viewer Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the FTXUI `LogView` component and the `streaming_log_viewer` interactive example application on top of PR7A's models.

**Architecture:** `LogView` wraps `VirtualList` and bridges either a `StreamingDocument` (raw-mode line view) or a `LogModel` (structured/ANSI-colored log view). A `render_styled_text` utility converts `StyledText` spans into FTXUI `Element` trees. Follow/pause is owned by `LogView` and auto-scrolls on revision change when follow is active. The example app runs a real-time generator thread that feeds either sink and demonstrates both modes.

**Tech Stack:** C++20, FTXUI, GoogleTest/CTest, CMake, existing `VirtualList`/`style_bridge`/`Document` targets.

## Global Constraints

- `LogView` lives in `components/` and links `TerminalUiKit::Components` + `TerminalUiKit::Document`.
- C++20 and the repository Google C++ Style configuration.
- Types use `PascalCase`, functions/locals use `snake_case`, and private fields end in `_`; files use `snake_case.h`/`.cc`.
- Follow is on by default; any explicit scroll-up (arrow up, wheel up) sets follow to false.
- A "follow" button or key toggles follow back on; default key is `End`.
- `LogView` polls the source `revision()` each frame; if follow is on and revision changed, it calls `scroll_to_bottom()`.
- `render_styled_text` lives in `style_bridge.h`/`.cc` since it depends on `to_decorator`.
- The example uses a background `std::thread` to simulate streaming input; the thread is joined on exit.

---

### Task 1: Add `render_styled_text` to style_bridge

**Files:**
- Modify: `include/terminal_ui_kit/components/style_bridge.h`
- Modify: `src/terminal_ui_kit/components/style_bridge.cc`
- Create: `tests/terminal_ui_kit/rendering/style_bridge_test.cc` (or modify existing if present)

**Interfaces:**
- Consumes: `StyledText` (from Core), `to_decorator(const TextStyle&)` (already in style_bridge)
- Produces: `ftxui::Element render_styled_text(const StyledText& text)` — renders `StyledText` spans as an `ftxui::hbox` of `ftxui::text` elements with decorators applied

- [ ] **Step 1: Add the declaration to style_bridge.h**

```cpp
// Converts a StyledText sequence into an FTXUI horizontal element, applying
// each span's TextStyle via to_decorator. Adjacent plain-text spans are
// coalesced into a single ftxui::text node.
ftxui::Element render_styled_text(const StyledText& text);
```

- [ ] **Step 2: Add the implementation to style_bridge.cc**

```cpp
ftxui::Element render_styled_text(const StyledText& text) {
  if (text.spans().empty()) {
    return ftxui::text("");
  }
  ftxui::Elements children;
  for (const auto& span : text.spans()) {
    children.push_back(ftxui::text(span.text) | to_decorator(span.style));
  }
  return ftxui::hbox(std::move(children));
}
```

- [ ] **Step 3: Write rendering tests**

```cpp
// in tests/terminal_ui_kit/rendering/style_bridge_test.cc (append to existing)

TEST(StyleBridge, RenderStyledTextEmptyReturnsEmpty) {
  StyledText text;
  ftxui::Element elem = render_styled_text(text);
  ftxui::Screen screen = test_support::render_to_screen(elem, 10, 1);
  EXPECT_EQ(screen.ToString(), "          ");
}

TEST(StyleBridge, RenderStyledTextBoldSpan) {
  StyledText text;
  TextStyle bold_style;
  bold_style.bold = true;
  text.append(TextSpan{"hello", bold_style});
  ftxui::Element elem = render_styled_text(text);
  ftxui::Screen screen = test_support::render_to_screen(elem, 10, 1);
  EXPECT_TRUE(screen.PixelAt(0, 0).bold);
  EXPECT_EQ(screen.PixelAt(0, 0).character, "h");
}

TEST(StyleBridge, RenderStyledTextMultipleSpans) {
  StyledText text;
  TextStyle bold_style;
  bold_style.bold = true;
  text.append(TextSpan{"ab", bold_style});
  text.append(TextSpan{"cd", TextStyle{}});
  std::string rendered = test_support::render_to_text(render_styled_text(text), 10, 1);
  rendered.erase(rendered.find_last_not_of(' ') + 1);
  EXPECT_EQ(rendered, "abcd");
}
```

- [ ] **Step 4: Run the focused rendering test, then the full rendering suite**

Run: `ctest --test-dir build-debug -R style_bridge -V`
Expected: all style_bridge tests pass

- [ ] **Step 5: Commit**

```bash
git add include/terminal_ui_kit/components/style_bridge.h src/terminal_ui_kit/components/style_bridge.cc tests/terminal_ui_kit/rendering/style_bridge_test.cc
git commit -m "feat: add render_styled_text to convert StyledText to FTXUI Element"
```

---

### Task 2: Implement LogView component

**Files:**
- Create: `include/terminal_ui_kit/components/log_view.h`
- Create: `src/terminal_ui_kit/components/log_view.cc`
- Modify: `src/terminal_ui_kit/CMakeLists.txt` — add `log_view.cc` to `terminal_ui_kit_components`
- Modify: `src/terminal_ui_kit/CMakeLists.txt` — link `terminal_ui_kit_document` to `terminal_ui_kit_components`

**Interfaces:**
- Consumes: `StreamingDocument*`, `LogModel*`, `VirtualListOptions`, `VirtualListModel`, `render_styled_text`, `to_decorator`, `default_dark_theme`
- Produces: `LogView` class with `component()` accessor and follow/pause control

```cpp
// log_view.h
#pragma once

#include <memory>

#include <ftxui/component/component.hpp>

#include "terminal_ui_kit/components/virtual_list.h"
#include "terminal_ui_kit/core/styled_text.h"
#include "terminal_ui_kit/document/log_model.h"
#include "terminal_ui_kit/document/streaming_document.h"
#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

struct LogViewOptions {
  StreamingDocument* document = nullptr;   // raw text mode
  LogModel* log_model = nullptr;           // structured log mode
  bool show_timestamp = true;
  bool show_severity = true;
  Theme theme = default_dark_theme();
};

class LogViewImpl;

class LogView {
 public:
  explicit LogView(LogViewOptions options);

  ftxui::Component component() const;

  bool follow() const;
  void set_follow(bool follow);
  void scroll_to_bottom();

 private:
  std::shared_ptr<LogViewImpl> impl_;
};

}  // namespace terminal_ui_kit
```

Implementation plan for `LogViewImpl`:
- Stores `VirtualListModel` internally
- On each render: checks source `revision()`, if follow is on and revision changed, calls `model_.scroll_to_bottom()`
- `render_item` builds a line from either `StreamingDocument::line_at()` (raw text mode) or `LogModel::at()` (structured mode with severity badge + timestamp + ANSI-colored message)
- Intercepts arrow-up and wheel-up events to set follow = false
- Intercepts `End` key to set follow = true
- Severity badge: colored prefix like `[TRACE]` / `[INFO]` / `[ERROR]` using theme colors

- [ ] **Step 1: Write the log_view.h header**

See interface above. Include guard, namespace, forward declarations.

- [ ] **Step 2: Write failing unit tests for LogView**

```cpp
// tests/terminal_ui_kit/rendering/log_view_test.cc
#include "terminal_ui_kit/components/log_view.h"
#include "terminal_ui_kit/document/log_model.h"
#include "terminal_ui_kit/document/streaming_document.h"
#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>
#include <ftxui/component/event.hpp>

namespace terminal_ui_kit {
namespace {

TEST(LogView, RawModeShowsStreamingLines) {
  StreamingDocument doc;
  doc.append("hello\nworld");
  doc.finish();

  LogViewOptions opts;
  opts.document = &doc;
  LogView view(opts);

  ftxui::Screen screen = test_support::render_to_screen(
      view.component()->Render(), 20, 3);
  std::string text = screen.ToString();
  EXPECT_NE(text.find("hello"), std::string::npos);
  EXPECT_NE(text.find("world"), std::string::npos);
}

TEST(LogView, StructuredModeShowsLogEntry) {
  LogModel model;
  LogEntry entry;
  entry.timestamp = "12:00";
  entry.severity = LogSeverity::kInfo;
  TextStyle style;
  style.bold = true;
  entry.message.append(TextSpan{"test message", style});
  model.append(std::move(entry));

  LogViewOptions opts;
  opts.log_model = &model;
  LogView view(opts);

  std::string text = test_support::render_to_text(
      view.component()->Render(), 30, 3);
  EXPECT_NE(text.find("test message"), std::string::npos);
}

TEST(LogView, FollowAutoScrollsOnNewData) {
  StreamingDocument doc;
  doc.append("line1\n");

  LogViewOptions opts;
  opts.document = &doc;
  opts.follow = true;
  LogView view(opts);

  // First render establishes content
  view.component()->Render();

  // Add new data
  doc.append("line2\n");
  doc.finish();

  // Rendering while follow is on should trigger scroll-to-bottom
  // We verify by checking the last visible line is "line2"
  // (indirect — the VirtualList scroll offset is internal)
  std::string text = test_support::render_to_text(
      view.component()->Render(), 20, 2);
  EXPECT_NE(text.find("line2"), std::string::npos);
}

TEST(LogView, ArrowUpDisablesFollow) {
  StreamingDocument doc;
  doc.append("line1\nline2\nline3\n");

  LogViewOptions opts;
  opts.document = &doc;
  opts.follow = true;
  LogView view(opts);

  view.component()->OnEvent(ftxui::Event::ArrowUp);
  EXPECT_FALSE(view.follow());
}

TEST(LogView, ArrowUpWhileNotFollowedLeavesFollowOff) {
  StreamingDocument doc;
  doc.append("line1\n");

  LogViewOptions opts;
  opts.document = &doc;
  opts.follow = false;
  LogView view(opts);

  view.component()->OnEvent(ftxui::Event::ArrowUp);
  EXPECT_FALSE(view.follow());
}

TEST(LogView, SetFollowReenablesFollowing) {
  StreamingDocument doc;
  doc.append("line1\n");

  LogViewOptions opts;
  opts.document = &doc;
  opts.follow = true;
  LogView view(opts);

  // Disable via scroll
  view.component()->OnEvent(ftxui::Event::ArrowUp);
  EXPECT_FALSE(view.follow());

  // Re-enable via End key
  view.component()->OnEvent(ftxui::Event::End);
  EXPECT_TRUE(view.follow());
}

}  // namespace
}  // namespace terminal_ui_kit
```

- [ ] **Step 3: Add rendering test registration to CMakeLists.txt**

Modify `tests/terminal_ui_kit/rendering/CMakeLists.txt`:
```cmake
add_executable(terminal_ui_kit_rendering_tests
  ...
  log_view_test.cc
  ...)
target_link_libraries(terminal_ui_kit_rendering_tests PRIVATE
  TerminalUiKit::Testing
  TerminalUiKit::Components
  TerminalUiKit::Document    # new — LogView needs LogModel/StreamingDocument
  GTest::gtest_main)
```

- [ ] **Step 4: Run the tests to confirm they fail**

Run: `cmake --build build-debug --target terminal_ui_kit_rendering_tests 2>&1 | head -20`
Expected: compile error — `log_view.h` not found

- [ ] **Step 5: Implement log_view.cc**

```cpp
#include "terminal_ui_kit/components/log_view.h"

#include <cstddef>
#include <string>
#include <utility>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/style_bridge.h"
#include "terminal_ui_kit/core/styled_text.h"
#include "terminal_ui_kit/document/log_model.h"
#include "terminal_ui_kit/document/streaming_document.h"
#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

namespace {

ftxui::Element severity_badge(LogSeverity severity, const Theme& theme) {
  std::string label;
  TextStyle badge_style;
  switch (severity) {
    case LogSeverity::kTrace:    label = " TRCE "; badge_style = theme.muted;     break;
    case LogSeverity::kDebug:    label = " DEBUG"; badge_style = theme.code;      break;
    case LogSeverity::kInfo:     label = " INFO "; badge_style = theme.accent;    break;
    case LogSeverity::kWarning:  label = " WARN "; badge_style = theme.warning;   break;
    case LogSeverity::kError:    label = " ERR ";  badge_style = theme.error;     break;
  }
  return ftxui::text(label) | to_decorator(badge_style) | ftxui::dim;
}

}  // namespace

class LogViewImpl {
 public:
  explicit LogViewImpl(LogViewOptions options)
      : options_(std::move(options)),
        last_revision_(0) {
    VirtualListOptions list_opts;
    list_opts.item_count = [this] { return source_size(); };
    list_opts.render_item = [this](std::size_t index, int) {
      return render_line(index);
    };

    // Wrap the internal model so we can intercept events
    model_ = std::make_shared<VirtualListModel>(std::move(list_opts));
    component_ = ftxui::Container::Vertical({});

    // Capture follow state changes from scroll events
    auto wrapped = model_->component();
    component_->Add(wrapped);
    component_ |= ftxui::CatchEvent([this](const ftxui::Event& event) {
      return handle_event(event);
    });
  }

  ftxui::Component component() const { return component_; }

  bool follow() const { return options_.follow; }
  void set_follow(bool follow) {
    options_.follow = follow;
    if (follow) {
      model_->scroll_to_bottom();
    }
  }
  void scroll_to_bottom() {
    model_->scroll_to_bottom();
  }

 private:
  std::size_t source_size() const {
    if (options_.document) return options_.document->line_count();
    if (options_.log_model) return options_.log_model->size();
    return 0;
  }

  ftxui::Element render_line(std::size_t index) {
    if (options_.document) {
      return ftxui::text(std::string(options_.document->line_at(index)));
    }
    if (options_.log_model) {
      const LogEntry& entry = options_.log_model->at(index);
      ftxui::Elements parts;
      if (options_.show_severity) {
        parts.push_back(severity_badge(entry.severity, options_.theme));
        parts.push_back(ftxui::text(" "));
      }
      if (options_.show_timestamp && !entry.timestamp.empty()) {
        parts.push_back(
            ftxui::text(entry.timestamp) | to_decorator(options_.theme.muted));
        parts.push_back(ftxui::text(" "));
      }
      parts.push_back(render_styled_text(entry.message));
      return ftxui::hbox(std::move(parts));
    }
    return ftxui::text("");
  }

  bool handle_event(const ftxui::Event& event) {
    // Arrow up and wheel up disable follow
    if (event == ftxui::Event::ArrowUp ||
        event == ftxui::Event::ArrowDown ||
        (event.is_mouse() && event.mouse().button == ftxui::Mouse::WheelUp) ||
        (event.is_mouse() && event.mouse().button == ftxui::Mouse::WheelDown)) {
      options_.follow = false;
    }
    // End re-enables follow
    if (event == ftxui::Event::End) {
      options_.follow = true;
      model_->scroll_to_bottom();
      return true;
    }
    return false;
  }

  LogViewOptions options_;
  std::shared_ptr<VirtualListModel> model_;
  ftxui::Component component_;
  std::uint64_t last_revision_ = 0;
};

LogView::LogView(LogViewOptions options)
    : impl_(std::make_shared<LogViewImpl>(std::move(options))) {}

ftxui::Component LogView::component() const { return impl_->component(); }
bool LogView::follow() const { return impl_->follow(); }
void LogView::set_follow(bool follow) { impl_->set_follow(follow); }
void LogView::scroll_to_bottom() { impl_->scroll_to_bottom(); }

}  // namespace terminal_ui_kit
```

- [ ] **Step 6: Update src/terminal_ui_kit/CMakeLists.txt**

Add `log_view.cc` to `terminal_ui_kit_components` sources:
```cmake
add_library(terminal_ui_kit_components
  ...
  components/log_view.cc
  ...)
```

Link `terminal_ui_kit_document` to `terminal_ui_kit_components`:
```cmake
target_link_libraries(terminal_ui_kit_components PUBLIC
  terminal_ui_kit_core
  terminal_ui_kit_document    # new — LogView needs LogModel/StreamingDocument
  ftxui::dom
  ftxui::component
  ftxui::screen)
```

- [ ] **Step 7: Build and run the focused tests**

Run: `cmake --build build-debug --target terminal_ui_kit_rendering_tests && ctest --test-dir build-debug -R log_view -V`
Expected: all LogView tests pass

- [ ] **Step 8: Commit**

```bash
git add include/terminal_ui_kit/components/log_view.h src/terminal_ui_kit/components/log_view.cc src/terminal_ui_kit/CMakeLists.txt tests/terminal_ui_kit/rendering/log_view_test.cc tests/terminal_ui_kit/rendering/CMakeLists.txt
git commit -m "feat: add LogView component with follow/pause and severity badge rendering"
```

---

### Task 3: Create streaming_log_viewer example application

**Files:**
- Create: `examples/streaming_log_viewer/CMakeLists.txt`
- Create: `examples/streaming_log_viewer/main.cc`
- Modify: `examples/CMakeLists.txt` — add `add_subdirectory(streaming_log_viewer)`

**Interfaces:**
- Consumes: `LogView`, `StreamingDocument`, `LogModel`, `LogEntry`, `parse_ansi`, `default_dark_theme`
- Produces: standalone interactive example binary

- [ ] **Step 1: Create CMakeLists.txt for the example**

```cmake
add_executable(terminal_ui_kit_example_streaming_log_viewer main.cc)

target_link_libraries(terminal_ui_kit_example_streaming_log_viewer PRIVATE
  TerminalUiKit::Components
  TerminalUiKit::Document
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_terminal_ui_kit_warnings(terminal_ui_kit_example_streaming_log_viewer)
```

- [ ] **Step 2: Register in examples/CMakeLists.txt**

Add: `add_subdirectory(streaming_log_viewer)` after `add_subdirectory(virtual_list_viewer)`

- [ ] **Step 3: Write the example main.cc**

```cpp
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <string>
#include <thread>
#include <utility>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/key_hint_bar.h"
#include "terminal_ui_kit/components/log_view.h"
#include "terminal_ui_kit/document/ansi_parser.h"
#include "terminal_ui_kit/document/log_model.h"
#include "terminal_ui_kit/document/streaming_document.h"
#include "terminal_ui_kit/theme/theme.h"

int main() {
  using namespace terminal_ui_kit;
  using namespace std::chrono_literals;

  const Theme& theme = default_dark_theme();
  StreamingDocument document;
  LogModel log_model;
  std::atomic<bool> running{true};

  LogViewOptions opts;
  opts.log_model = &log_model;
  opts.theme = theme;
  LogView view(std::move(opts));

  // Generator thread: produces log entries with ANSI color
  std::thread generator([&] {
    const char* severities[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR"};
    const LogSeverity sev_map[] = {
        LogSeverity::kTrace, LogSeverity::kDebug, LogSeverity::kInfo,
        LogSeverity::kWarning, LogSeverity::kError};
    const char* messages[] = {
        "\x1b[32mConnection\x1b[0m established to 10.0.0.1:8080",
        "Received \x1b[33m42\x1b[0m bytes from upstream",
        "\x1b[31mError\x1b[0m: timeout waiting for acknowledgement",
        "Processing batch \x1b[36m#1024\x1b[0m",
        "Cache hit ratio: \x1b[32m87.3%\x1b[0m",
        "\x1b[33mWarning\x1b[0m: memory usage at 78%",
        "Request \x1b[36mGET /api/v2/users\x1b[0m completed in 143ms",
        "Retry attempt \x1b[33m3/5\x1b[0m for job-id=abc-123",
        "\x1b[32mSuccess\x1b[0m: migration task finished",
        "Heartbeat check: \x1b[32mOK\x1b[0m",
    };
    constexpr std::size_t kMsgCount = sizeof(messages) / sizeof(messages[0]);

    std::srand(static_cast<unsigned>(std::time(nullptr)));
    for (std::size_t i = 0; running; ++i) {
      std::this_thread::sleep_for(200ms + std::chrono::milliseconds(std::rand() % 300));

      auto now = std::chrono::system_clock::now();
      auto tt = std::chrono::system_clock::to_time_t(now);
      auto tm = std::localtime(&tt);
      char timestamp[16];
      std::strftime(timestamp, sizeof(timestamp), "%H:%M:%S", tm);

      int sev_idx = std::rand() % 5;
      LogEntry entry;
      entry.timestamp = timestamp;
      entry.severity = sev_map[sev_idx];
      entry.message = parse_ansi(messages[std::rand() % kMsgCount]);
      log_model.append(std::move(entry));
    }
  });

  auto screen = ftxui::ScreenInteractive::Fullscreen();
  ftxui::Component root = ftxui::Renderer(view.component(), [&] {
    return ftxui::vbox({
               ftxui::text("Terminal UI Kit - Streaming Log Viewer") | ftxui::bold,
               ftxui::text("Watch live logs from a simulated backend") | ftxui::dim,
               ftxui::separator(),
               view.component()->Render() | ftxui::flex,
               ftxui::separator(),
               ftxui::text(std::string("Follow: ") + (view.follow() ? "ON" : "OFF")) |
                   (view.follow() ? ftxui::color(ftxui::Color::Green) : ftxui::color(ftxui::Color::GrayDark)),
               KeyHintBar({{"up/down", "scroll"},
                           {"end", "follow"},
                           {"q/ESC", "quit"}},
                          theme),
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
  running = false;
  generator.join();
}
```

- [ ] **Step 4: Build and run the example**

Run: `cmake --build build-debug --target terminal_ui_kit_example_streaming_log_viewer`
Expected: binary compiles and links successfully

- [ ] **Step 5: Verify the example runs**

Run: `echo q | timeout 3 ./build-debug/examples/streaming_log_viewer/terminal_ui_kit_example_streaming_log_viewer 2>&1`
Expected: starts, shows log entries, exits on 'q'

- [ ] **Step 6: Commit**

```bash
git add examples/streaming_log_viewer/ examples/CMakeLists.txt
git commit -m "feat: add streaming_log_viewer example with live log generation"
```

---

### Task 4: Final verification and handoff

- [ ] **Step 1: Build full suite and run all tests**

Run: `cmake --build build-debug && ctest --test-dir build-debug --output-on-failure`
Expected: all tests pass (unit + rendering)

- [ ] **Step 2: Run clang-format on all changed files**

Run: `git clang-format --style=file HEAD~3`
Expected: no formatting errors

- [ ] **Step 3: Verify the full diff**

Run: `git diff main..HEAD --stat`
Expected: clean diff with only the intended files changed

- [ ] **Step 4: Update changelog**

Modify `docs/modules/ROOT/pages/changelog.adoc` under `[Unreleased]` → `Added`:
```
- LogView component with follow/pause and severity badges (#PR7B)
- streaming_log_viewer interactive example application
- render_styled_text utility for StyledText-to-FTXUI Element conversion
```

- [ ] **Step 5: Commit changelog**

```bash
git add docs/modules/ROOT/pages/changelog.adoc
git commit -m "docs: document PR7B LogView and streaming viewer"
```
