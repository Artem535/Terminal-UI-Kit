// Example: UnifiedDiffView
//
// A standalone, interactive demo of the virtualized unified-diff view. It
// loads deterministic in-memory diff scenarios (no network, no external
// service) and lets you scroll, navigate hunks/files, collapse/expand, search,
// and copy diff lines. Use Tab to switch scenario and q / Esc to exit.
//
// Controls (the diff view handles the diff-specific keys; the example handles
// Tab, q, and Esc):
//   j / k or arrows  Scroll
//   n / N            Next / previous hunk (or search result while searching)
//   ] / [            Next / previous file
//   Enter            Collapse / expand the selected file
//   /                Search (type, Enter to jump, Esc to cancel)
//   y                Invoke the copy callback for the selected line
//   Tab              Switch scenario
//   q / Esc          Exit

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/unified_diff_view.h"
#include "terminal_ui_kit/diff/diff_model.h"
#include "terminal_ui_kit/diff/unified_diff_parser.h"

using terminal_ui_kit::UnifiedDiffView;
using terminal_ui_kit::UnifiedDiffViewOptions;
using terminal_ui_kit::diff::DiffFile;
using terminal_ui_kit::diff::UnifiedDiffParser;

namespace {

const char* kNormal =
    "diff --git a/src/app.cc b/src/app.cc\n"
    "index 1111111..2222222 100644\n"
    "--- a/src/app.cc\n"
    "+++ b/src/app.cc\n"
    "@@ -1,6 +1,8 @@\n"
    " #include <iostream>\n"
    " \n"
    " int main() {\n"
    "-  std::cout << \"hello old\" << std::endl;\n"
    "+  std::cout << \"hello new\" << std::endl;\n"
    "+  std::cout << \"extra line\" << std::endl;\n"
    "   return 0;\n"
    " }\n";

const char* kMultiFile =
    "diff --git a/one.txt b/one.txt\n"
    "--- a/one.txt\n"
    "+++ b/one.txt\n"
    "@@ -1,2 +1,3 @@\n"
    " apple\n"
    " banana\n"
    "+cherry\n"
    "diff --git a/two.txt b/two.txt\n"
    "--- a/two.txt\n"
    "+++ b/two.txt\n"
    "@@ -1,2 +1,2 @@\n"
    "-red\n"
    "+green\n"
    " blue\n";

const char* kNewFile =
    "diff --git a/README.md b/README.md\n"
    "new file mode 100644\n"
    "index 0000000..abcdef1\n"
    "--- /dev/null\n"
    "+++ b/README.md\n"
    "@@ -0,0 +1,3 @@\n"
    "+# New Project\n"
    "+\n"
    "+A freshly added file.\n";

const char* kDeletedFile =
    "diff --git a/old_notes.txt b/old_notes.txt\n"
    "deleted file mode 100644\n"
    "index abcdef1..0000000\n"
    "--- a/old_notes.txt\n"
    "+++ /dev/null\n"
    "@@ -1,2 +0,0 @@\n"
    "-obsolete\n"
    "-removed\n";

const char* kBinaryFile =
    "diff --git a/assets/logo.png b/assets/logo.png\n"
    "index 1111111..2222222 100644\n"
    "Binary files a/assets/logo.png and b/assets/logo.png differ\n";

const char* kMultiHunk =
    "diff --git a/src/engine.cc b/src/engine.cc\n"
    "--- a/src/engine.cc\n"
    "+++ b/src/engine.cc\n"
    "@@ -10,3 +10,3 @@ void setup() {\n"
    " init();\n"
    " bind();\n"
    "-poll();\n"
    "+poll_now();\n"
    "@@ -40,3 +40,5 @@ void teardown() {\n"
    " flush();\n"
    " close();\n"
    "+notify();\n"
    "+done = true;\n";

const char* kLongLines =
    "diff --git a/data/big_record.txt b/data/big_record.txt\n"
    "--- a/data/big_record.txt\n"
    "+++ b/data/big_record.txt\n"
    "@@ -1,2 +1,2 @@\n"
    "-This is a very long line that exceeds any reasonable terminal width so the "
    "view must truncate it with a continuation marker rather than wrap or render "
    "the whole thing across the whole line\n"
    "+This is a very long line that now has a slightly different tail that still "
    "needs to be truncated for display in a narrow terminal viewport\n";

const char* kEmptyDiff = "";

// Builds a single-file diff model with `line_count` context lines and one
// added line, so the hunk has `line_count + 1` lines.
std::vector<DiffFile> MakeLargeDiff(std::size_t line_count) {
  DiffFile file;
  file.old_path = "huge/generated.txt";
  file.new_path = "huge/generated.txt";
  terminal_ui_kit::diff::DiffHunk hunk;
  hunk.header =
      "@@ -1," + std::to_string(line_count) + " +1," + std::to_string(line_count + 1) + " @@";
  for (std::size_t i = 1; i <= line_count; ++i) {
    terminal_ui_kit::diff::DiffLine line;
    line.type = terminal_ui_kit::diff::DiffLineType::kContext;
    line.old_line = static_cast<int>(i);
    line.new_line = static_cast<int>(i);
    line.content.append(terminal_ui_kit::TextSpan{"generated line " + std::to_string(i),
                                                  terminal_ui_kit::TextStyle{}, std::nullopt});
    hunk.lines.push_back(std::move(line));
  }
  terminal_ui_kit::diff::DiffLine added;
  added.type = terminal_ui_kit::diff::DiffLineType::kAdded;
  added.new_line = static_cast<int>(line_count) + 1;
  added.content.append(terminal_ui_kit::TextSpan{"+ trailing added line",
                                                 terminal_ui_kit::TextStyle{}, std::nullopt});
  hunk.lines.push_back(std::move(added));
  file.hunks.push_back(std::move(hunk));

  std::vector<DiffFile> files;
  files.push_back(std::move(file));
  return files;
}

struct Scenario {
  std::string name;
  std::vector<DiffFile> files;
};

std::vector<Scenario> MakeScenarios() {
  std::vector<Scenario> scenarios;
  scenarios.push_back({"Normal single-file diff", UnifiedDiffParser{}.Parse(kNormal)});
  scenarios.push_back({"Multi-file diff", UnifiedDiffParser{}.Parse(kMultiFile)});
  scenarios.push_back({"New file", UnifiedDiffParser{}.Parse(kNewFile)});
  scenarios.push_back({"Deleted file", UnifiedDiffParser{}.Parse(kDeletedFile)});
  scenarios.push_back({"Binary file", UnifiedDiffParser{}.Parse(kBinaryFile)});
  scenarios.push_back({"Multiple hunks", UnifiedDiffParser{}.Parse(kMultiHunk)});
  scenarios.push_back({"Long lines", UnifiedDiffParser{}.Parse(kLongLines)});
  scenarios.push_back({"Empty diff", UnifiedDiffParser{}.Parse(kEmptyDiff)});
  scenarios.push_back({"Large diff (100,000 lines)", MakeLargeDiff(100000)});
  return scenarios;
}

}  // namespace

int main() {
  std::vector<Scenario> scenarios = MakeScenarios();
  std::size_t current = 0;

  std::string copied;
  UnifiedDiffViewOptions options;
  options.on_copy = [&copied](std::string text) { copied = std::move(text); };
  UnifiedDiffView view(std::move(options));
  view.SetFiles(scenarios[current].files);

  auto screen = ftxui::ScreenInteractive::Fullscreen();

  auto header = ftxui::Renderer([&] {
    return ftxui::hbox({
        ftxui::text("UnifiedDiffView — ") | ftxui::bold,
        ftxui::text(scenarios[current].name),
        ftxui::text("  [j/k scroll  n/N hunk  ]/[ file  Enter collapse  / "
                    "search  y copy  Tab scenario  q quit]") |
            ftxui::color(ftxui::Color::GrayDark),
    });
  });

  auto footer = ftxui::Renderer([&] {
    std::string text = "Copied: ";
    text += copied.empty() ? "(nothing yet — move to a line and press y)" : copied;
    return ftxui::text(text) | ftxui::color(ftxui::Color::GrayDark);
  });

  auto layout = ftxui::Container::Vertical({
      header,
      view.component(),
      footer,
  });

  auto component = ftxui::CatchEvent(layout, [&](ftxui::Event event) {
    if (event == ftxui::Event::Character('q') || event == ftxui::Event::Escape) {
      screen.Exit();
      return true;
    }
    if (event == ftxui::Event::Tab) {
      current = (current + 1) % scenarios.size();
      copied.clear();
      view.SetFiles(scenarios[current].files);
      return true;
    }
    return false;
  });

  screen.Loop(component);
  return 0;
}
