// Example: UnifiedDiffView
//
// A standalone, interactive demo of the reusable unified-diff view component
// (terminal_ui_kit::UnifiedDiffView). It uses only the component's public API
// and the canonical UnifiedDiffParser / diff model, and demonstrates every
// primary user-facing behaviour on deterministic, self-contained sample data
// (no network access, no external services).
//
// Controls:
//   j / k  or  Up / Down   scroll / select a row
//   n / N                  next / previous hunk
//   ] / [                  next / previous file
//   Enter                  collapse / expand the current file
//   /                      search (Enter cycles matches, Esc closes)
//   y                      invoke the copy callback on the selected line
//   Tab                    switch to the next scenario
//   q / Esc                quit
//
// The view is virtualized: only the rows visible in the viewport are rendered
// each frame, so the included 100,000-line scenario stays responsive.

#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/key_hint_bar.h"
#include "terminal_ui_kit/components/unified_diff_view.h"
#include "terminal_ui_kit/diff/unified_diff_parser.h"
#include "terminal_ui_kit/theme/theme.h"

namespace {

using terminal_ui_kit::UnifiedDiffView;
using terminal_ui_kit::UnifiedDiffViewOptions;
using terminal_ui_kit::diff::DiffFile;
using terminal_ui_kit::diff::UnifiedDiffParser;

// A named deterministic scenario: a unified diff plus a short description.
struct Scenario {
  std::string name;
  std::string description;
  std::string diff;
};

const char* kNormalDiff = R"(diff --git a/hello.c b/hello.c
index 1111111..2222222 100644
--- a/hello.c
+++ b/hello.c
@@ -1,5 +1,6 @@
 #include <stdio.h>
 
 int main(void) {
-    printf("hello\n");
-    return 0;
+    printf("hello, world\n");
+    printf("have a nice day\n");
+    return 0;
 }
)";

const char* kMultiFileDiff = R"(diff --git a/src/app.c b/src/app.c
--- a/src/app.c
+++ b/src/app.c
@@ -12,3 +12,4 @@
     run();
+    cleanup();
 }
diff --git a/src/util.c b/src/util.c
--- a/src/util.c
+++ b/src/util.c
@@ -1,1 +1,1 @@
-old_util();
+new_util();
diff --git a/README.md b/README.md
--- a/README.md
+++ b/README.md
@@ -3,2 +3,3 @@
 # Project
-Installation instructions.
+Installation and usage instructions.
+See docs/.
)";

const char* kNewFileDiff = R"(diff --git a/new_file.txt b/new_file.txt
new file mode 100644
index 0000000..abcdef1
--- /dev/null
+++ b/new_file.txt
@@ -0,0 +1,4 @@
+The quick brown fox
+jumps over the lazy dog.
+Added entirely.
+Last line.
)";

const char* kDeletedFileDiff = R"(diff --git a/old_file.txt b/old_file.txt
deleted file mode 100644
index abcdef1..0000000
--- a/old_file.txt
+++ /dev/null
@@ -1,3 +0,0 @@
-This file is removed.
-Both lines go away.
-Done.
)";

const char* kBinaryFileDiff = R"(diff --git a/logo.png b/logo.png
index 1111111..2222222 100644
Binary files a/logo.png and b/logo.png differ
)";

const char* kMultipleHunksDiff = R"(diff --git a/parser.c b/parser.c
--- a/parser.c
+++ b/parser.c
@@ -20,4 +20,5 @@
     token = next();
+    skip_ws();
     return token;
 }
@@ -40,3 +40,4 @@
 case TOK_NUMBER:
     value = parse_number();
+    seen = true;
     break;
@@ -60,2 +60,3 @@
 static void reset(void) {
+    state = 0;
 }
)";

const char* kLongLinesDiff = R"(diff --git a/generated.c b/generated.c
--- a/generated.c
+++ b/generated.c
@@ -1,2 +1,2 @@
-const char* data = "this is an extremely long line of generated content that will definitely exceed the width of any ordinary terminal viewport column budget";
+const char* data = "and here is the replacement long line that is equally very long and also spills far beyond the viewport width";
)";

const char* kEmptyDiff = "";

// Builds a deterministic diff with ~100,000 diff lines across ten files.
std::string make_large_diff() {
  constexpr int kFiles = 10;
  constexpr int kContext = 5000;
  std::string diff;
  for (int f = 0; f < kFiles; ++f) {
    char name[32];
    std::snprintf(name, sizeof(name), "big_%02d.txt", f);
    diff += "diff --git a/";
    diff += name;
    diff += " b/";
    diff += name;
    diff += "\n--- a/";
    diff += name;
    diff += "\n+++ b/";
    diff += name;
    diff += "\n@@ -1,";
    diff += std::to_string(kContext);
    diff += " +1,";
    diff += std::to_string(kContext * 2);
    diff += " @@\n";
    for (int i = 0; i < kContext; ++i) {
      diff += " line_";
      diff += std::to_string(i);
      diff += "\n+added_";
      diff += std::to_string(i);
      diff += "\n";
    }
  }
  return diff;
}

std::vector<Scenario> make_scenarios() {
  std::vector<Scenario> scenarios;
  scenarios.push_back({"Normal", "one file, one hunk", kNormalDiff});
  scenarios.push_back({"Multi-file", "three modified files", kMultiFileDiff});
  scenarios.push_back({"New file", "a file added entirely", kNewFileDiff});
  scenarios.push_back({"Deleted file", "a file removed entirely", kDeletedFileDiff});
  scenarios.push_back({"Binary", "a binary file that differs", kBinaryFileDiff});
  scenarios.push_back({"Multiple hunks", "three hunks in one file", kMultipleHunksDiff});
  scenarios.push_back({"Long lines", "content wider than the viewport", kLongLinesDiff});
  scenarios.push_back({"Empty diff", "no changes at all", kEmptyDiff});
  scenarios.push_back({"100k lines", "ten files, ~100,000 diff lines", make_large_diff()});
  return scenarios;
}

}  // namespace

int main() {
  using namespace ftxui;
  using namespace terminal_ui_kit;

  const Theme& theme = default_dark_theme();
  const std::vector<Scenario> scenarios = make_scenarios();

  std::size_t scenario_index = 0;
  bool search_open = false;
  std::string search_query;
  std::string copy_status = "(press y to copy the selected line)";
  std::string last_copied;

  auto screen = ScreenInteractive::Fullscreen();

  // Rebuild the view with the model for `index`.
  auto make_view = [&](std::size_t index) {
    UnifiedDiffViewOptions opts;
    opts.theme = theme;
    opts.on_copy = [&](std::string text) { last_copied = std::move(text); };
    return std::make_shared<UnifiedDiffView>(UnifiedDiffParser{}.Parse(scenarios[index].diff),
                                             std::move(opts));
  };
  auto view = make_view(scenario_index);

  auto root = Renderer([&] {
    const UnifiedDiffView::Status s = view->status();
    const std::string hunk_text =
        (s.hunk_count == 0)
            ? "Hunk -"
            : ("Hunk " + std::to_string(s.hunk_index + 1) + "/" + std::to_string(s.hunk_count));
    const std::string visible_text =
        (s.row_count == 0)
            ? "Visible rows 0"
            : ("Visible rows " + std::to_string(s.first_visible + 1) + "-" +
               std::to_string(s.last_visible + 1) + " of " + std::to_string(s.row_count));

    Elements status_parts = {
        text("File " + std::to_string(s.file_index + 1) + "/" + std::to_string(s.file_count)) |
            color(ftxui::Color::Green),
        text(" "),
        text(hunk_text) | color(ftxui::Color::Yellow),
        text(" "),
        text(visible_text) | color(ftxui::Color::Cyan),
    };
    if (search_open) {
      status_parts.push_back(text("  |  Search: " + search_query));
      if (view->has_matches()) {
        status_parts.push_back(text(" [" + std::to_string((view->current_match().value_or(0)) + 1) +
                                    "/" + std::to_string(view->match_count()) + " matches]"));
      } else {
        status_parts.push_back(text(" [no matches]") | color(ftxui::Color::Red));
      }
    }

    Elements children = {
        hbox({text(" Terminal UI Kit - UnifiedDiffView ") | bold,
              text("  Scenario " + std::to_string(scenario_index + 1) + "/" +
                   std::to_string(scenarios.size()) + ": " + scenarios[scenario_index].name + " (" +
                   scenarios[scenario_index].description + ")") |
                  dim}),
        separator(),
        view->component()->Render() | flex,
        separator(),
        hbox(std::move(status_parts)),
        hbox({text("Last copy: ") | dim, text(last_copied) | color(ftxui::Color::Magenta)}) | dim,
        KeyHintBar({{"j/k ↑↓", "scroll"},
                    {"n/N", "hunk"},
                    {"]/[", "file"},
                    {"Enter", "collapse"},
                    {"/", "search"},
                    {"y", "copy"},
                    {"Tab", "scenario"},
                    {"q/Esc", "quit"}},
                   theme),
    };

    return vbox(std::move(children)) | border;
  });

  root |= CatchEvent([&](Event event) -> bool {
    if (search_open) {
      if (event == Event::Escape) {
        search_open = false;
        return true;
      }
      if (event == Event::Return) {
        if (view->has_matches()) view->jump_to_next_match();
        return true;
      }
      if (event == Event::Backspace) {
        if (!search_query.empty()) {
          search_query.pop_back();
          view->set_search(search_query);
        }
        return true;
      }
      if (event.is_character()) {
        search_query += event.character();
        view->set_search(search_query);
        return true;
      }
      return true;  // swallow other keys while searching
    }

    if (event == Event::Character('q') || event == Event::Escape) {
      screen.ExitLoopClosure()();
      return true;
    }
    if (event == Event::Tab) {
      scenario_index = (scenario_index + 1) % scenarios.size();
      view = make_view(scenario_index);
      last_copied.clear();
      return true;
    }
    if (event == Event::Character('/')) {
      search_open = true;
      search_query.clear();
      return true;
    }
    // Everything else goes to the diff view component.
    return view->component()->OnEvent(event);
  });

  screen.Loop(root);
  return 0;
}