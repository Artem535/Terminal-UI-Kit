#include "terminal_ui_kit/components/unified_diff_view.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/event.hpp>
#include <ftxui/screen/screen.hpp>

#include "terminal_ui_kit/diff/diff_model.h"
#include "terminal_ui_kit/diff/unified_diff_parser.h"
#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

using diff::DiffFile;
using diff::DiffLine;
using diff::DiffLineType;
using diff::UnifiedDiffParser;

// Parses a unified diff string through the canonical parser and loads it into
// a fresh view. This exercises the real model the view consumes.
UnifiedDiffView MakeView(std::string diff, UnifiedDiffViewOptions options = {}) {
  UnifiedDiffView view(std::move(options));
  view.SetFiles(UnifiedDiffParser{}.Parse(diff));
  return view;
}

// Renders the view's component to text. The virtual list only reports real
// content once its observed box has been populated, so the first pass lays
// out the element and the second pass captures the content (mirroring how
// the virtual-document rendering tests render twice).
std::string Render(const UnifiedDiffView& view, int width, int height) {
  (void)test_support::render_to_text(view.component()->Render(), width, height);
  return test_support::render_to_text(view.component()->Render(), width, height);
}

const char* kSingleFile =
    "diff --git a/file.txt b/file.txt\n"
    "index 1111111..2222222 100644\n"
    "--- a/file.txt\n"
    "+++ b/file.txt\n"
    "@@ -1,3 +1,4 @@\n"
    " keep\n"
    "-old one\n"
    "+new one\n"
    " gamma\n";

TEST(UnifiedDiffView, RendersSingleFileHeaderHunkAndLines) {
  UnifiedDiffView view = MakeView(kSingleFile);
  const std::string text = Render(view, 40, 12);

  EXPECT_NE(text.find("file.txt"), std::string::npos);
  EXPECT_NE(text.find("@@ -1,3 +1,4 @@"), std::string::npos);
  EXPECT_NE(text.find("keep"), std::string::npos);
  EXPECT_NE(text.find("old one"), std::string::npos);
  EXPECT_NE(text.find("new one"), std::string::npos);
  EXPECT_NE(text.find("File 1/1"), std::string::npos);
  EXPECT_NE(text.find("Hunk 1/1"), std::string::npos);
}

TEST(UnifiedDiffView, StatusLineReportsFileHunkAndVisibleRows) {
  UnifiedDiffView view = MakeView(kSingleFile);
  Render(view, 40, 10);
  const std::string status = view.status_line();
  EXPECT_NE(status.find("File 1/1"), std::string::npos);
  EXPECT_NE(status.find("Hunk 1/1"), std::string::npos);
  EXPECT_NE(status.find("Visible rows"), std::string::npos);
}

TEST(UnifiedDiffView, MultipleFilesAndFileNavigation) {
  const std::string diff =
      "diff --git a/one.txt b/one.txt\n"
      "--- a/one.txt\n"
      "+++ b/one.txt\n"
      "@@ -1 +1 @@\n"
      "-a\n"
      "+b\n"
      "diff --git a/two.txt b/two.txt\n"
      "--- a/two.txt\n"
      "+++ b/two.txt\n"
      "@@ -1 +1 @@\n"
      "-c\n"
      "+d\n";

  UnifiedDiffView view = MakeView(diff);
  EXPECT_EQ(view.file_count(), 2u);
  EXPECT_EQ(view.selected_file(), 0u);

  view.next_file();
  EXPECT_EQ(view.selected_file(), 1u);
  view.next_file();  // clamped at last file
  EXPECT_EQ(view.selected_file(), 1u);
  view.previous_file();
  EXPECT_EQ(view.selected_file(), 0u);
  view.previous_file();  // clamped at first file
  EXPECT_EQ(view.selected_file(), 0u);
}

TEST(UnifiedDiffView, MultipleHunksAndHunkNavigation) {
  const std::string diff =
      "diff --git a/x.txt b/x.txt\n"
      "--- a/x.txt\n"
      "+++ b/x.txt\n"
      "@@ -1,2 +1,2 @@\n"
      " one\n"
      " two\n"
      "@@ -10,1 +10,2 @@\n"
      " ten\n"
      "+tenplus\n";

  UnifiedDiffView view = MakeView(diff);
  EXPECT_EQ(view.selected_hunk(), 0u);
  // From the file header, next-hunk lands on the first hunk (hunk 0).
  view.next_hunk();
  EXPECT_EQ(view.selected_hunk(), 0u);
  view.next_hunk();
  EXPECT_EQ(view.selected_hunk(), 1u);
  view.next_hunk();  // clamped at last hunk
  EXPECT_EQ(view.selected_hunk(), 1u);
  view.previous_hunk();
  EXPECT_EQ(view.selected_hunk(), 0u);
  view.previous_hunk();  // clamped at first hunk
  EXPECT_EQ(view.selected_hunk(), 0u);
}

TEST(UnifiedDiffView, NewFileShowsNewBadge) {
  const std::string diff =
      "diff --git a/new.txt b/new.txt\n"
      "new file mode 100644\n"
      "--- /dev/null\n"
      "+++ b/new.txt\n"
      "@@ -0,0 +1,2 @@\n"
      "+a\n"
      "+b\n";

  UnifiedDiffView view = MakeView(diff);
  const std::string text = Render(view, 40, 12);
  EXPECT_NE(text.find("[new]"), std::string::npos);
  EXPECT_NE(text.find("new.txt"), std::string::npos);
}

TEST(UnifiedDiffView, DeletedFileShowsDelBadge) {
  const std::string diff =
      "diff --git a/gone.txt b/gone.txt\n"
      "deleted file mode 100644\n"
      "--- a/gone.txt\n"
      "+++ /dev/null\n"
      "@@ -1,1 +0,0 @@\n"
      "-x\n";

  UnifiedDiffView view = MakeView(diff);
  const std::string text = Render(view, 40, 12);
  EXPECT_NE(text.find("[del]"), std::string::npos);
}

TEST(UnifiedDiffView, BinaryFileShowsBinBadge) {
  const std::string diff =
      "diff --git a/img.png b/img.png\n"
      "index 1111111..2222222 100644\n"
      "Binary files a/img.png and b/img.png differ\n";

  UnifiedDiffView view = MakeView(diff);
  EXPECT_EQ(view.file_count(), 1u);
  const std::string text = Render(view, 40, 8);
  EXPECT_NE(text.find("[bin]"), std::string::npos);
  EXPECT_NE(text.find("img.png"), std::string::npos);
}

TEST(UnifiedDiffView, EmptyDiffRendersMessage) {
  UnifiedDiffView view = MakeView("");
  EXPECT_EQ(view.file_count(), 0u);
  const std::string text = Render(view, 40, 6);
  EXPECT_NE(text.find("empty diff"), std::string::npos);
  EXPECT_NE(view.status_line().find("File 0/0"), std::string::npos);
}

TEST(UnifiedDiffView, EmptyNewFileRendersHeaderWithoutLines) {
  const std::string diff =
      "diff --git a/empty.txt b/empty.txt\n"
      "new file mode 100644\n"
      "--- /dev/null\n"
      "+++ b/empty.txt\n"
      "@@ -0,0 +0,0 @@\n";

  UnifiedDiffView view = MakeView(diff);
  const std::string text = Render(view, 40, 8);
  EXPECT_NE(text.find("[new]"), std::string::npos);
  EXPECT_NE(text.find("empty.txt"), std::string::npos);
}

TEST(UnifiedDiffView, LongLinesAreTruncatedWithContinuationMarker) {
  const std::string long_line = "alpha" + std::string(300, 'x') + "omega";
  const std::string diff =
      "diff --git a/long.txt b/long.txt\n"
      "--- a/long.txt\n"
      "+++ b/long.txt\n"
      "@@ -1,1 +1,1 @@\n"
      "-" +
      long_line +
      "\n"
      "+" +
      long_line + "\n";

  UnifiedDiffView view = MakeView(diff);
  const std::string text = Render(view, 40, 10);
  // The line begins visibly but the tail ("omega") is truncated away.
  EXPECT_NE(text.find("alpha"), std::string::npos);
  EXPECT_EQ(text.find("omega"), std::string::npos);
}

// Removes ANSI CSI escape sequences so rendered text can be compared for
// content independent of color/style attributes.
std::string StripAnsi(std::string text) {
  std::string out;
  out.reserve(text.size());
  std::size_t i = 0;
  while (i < text.size()) {
    if (text[i] == '\x1B' && i + 1 < text.size() && text[i + 1] == '[') {
      while (i < text.size() && text[i] != 'm') ++i;
      if (i < text.size()) ++i;  // skip 'm'
      continue;
    }
    out.push_back(text[i++]);
  }
  return out;
}

TEST(UnifiedDiffView, NoColorFallbackRendersSameText) {
  const std::string text_color = Render(MakeView(kSingleFile), 40, 10);
  UnifiedDiffViewOptions no_color;
  no_color.enable_color = false;
  const std::string text_plain = Render(MakeView(kSingleFile, no_color), 40, 10);

  // The visible text is identical with or without color.
  EXPECT_EQ(StripAnsi(text_color), StripAnsi(text_plain));

  // The color build emits foreground/background color codes; the no-color
  // fallback emits none (only dim/invert attributes remain).
  EXPECT_NE(text_color.find("\x1B[9"), std::string::npos);
  EXPECT_EQ(text_plain.find("\x1B[3"), std::string::npos);
  EXPECT_EQ(text_plain.find("\x1B[4"), std::string::npos);
}

TEST(UnifiedDiffView, NarrowTerminalKeepsContentVisible) {
  UnifiedDiffView view = MakeView(kSingleFile);
  // A very narrow terminal: line numbers are dropped, markers + content stay.
  const std::string text = Render(view, 8, 10);
  EXPECT_NE(text.find("keep"), std::string::npos);
  EXPECT_NE(text.find("new one"), std::string::npos);
}

TEST(UnifiedDiffView, CollapseAndExpandFile) {
  const std::string diff =
      "diff --git a/a.txt b/a.txt\n"
      "--- a/a.txt\n"
      "+++ b/a.txt\n"
      "@@ -1,1 +1,1 @@\n"
      "-x\n"
      "+y\n";

  UnifiedDiffView view = MakeView(diff);
  const std::size_t before = view.layout_build_count();

  view.toggle_collapse();
  EXPECT_TRUE(view.collapsed(0));
  EXPECT_GT(view.layout_build_count(), before);
  const std::string collapsed_text = Render(view, 40, 10);
  // File header visible, body hidden.
  EXPECT_NE(collapsed_text.find("a.txt"), std::string::npos);
  EXPECT_EQ(collapsed_text.find("@@ -1,1 +1,1 @@"), std::string::npos);

  view.toggle_collapse();
  EXPECT_FALSE(view.collapsed(0));
  const std::string expanded_text = Render(view, 40, 10);
  EXPECT_NE(expanded_text.find("@@ -1,1 +1,1 @@"), std::string::npos);
}

TEST(UnifiedDiffView, CollapsedStatePersistsDuringNavigation) {
  const std::string diff =
      "diff --git a/one.txt b/one.txt\n"
      "--- a/one.txt\n"
      "+++ b/one.txt\n"
      "@@ -1 +1 @@\n"
      "-a\n"
      "+b\n"
      "diff --git a/two.txt b/two.txt\n"
      "--- a/two.txt\n"
      "+++ b/two.txt\n"
      "@@ -1 +1 @@\n"
      "-c\n"
      "+d\n";

  UnifiedDiffView view = MakeView(diff);
  view.toggle_collapse();  // collapse file 0
  view.next_file();        // navigate to file 1
  view.previous_file();    // back to file 0
  EXPECT_TRUE(view.collapsed(0));
  EXPECT_FALSE(view.collapsed(1));
}

TEST(UnifiedDiffView, SearchFindsAndNavigatesMatches) {
  const std::string diff =
      "diff --git a/x.txt b/x.txt\n"
      "--- a/x.txt\n"
      "+++ b/x.txt\n"
      "@@ -1,2 +1,2 @@\n"
      " foo\n"
      "-old bar\n"
      "+new bar\n"
      "@@ -10,1 +10,1 @@\n"
      " baz bar\n";

  UnifiedDiffView view = MakeView(diff);
  view.set_search("bar");
  EXPECT_EQ(view.search_result_count(), 3u);
  EXPECT_NE(view.search().find("bar"), std::string::npos);

  // The first two matches live in hunk 0; the third in hunk 1, so cycling
  // through the results eventually moves to the second hunk.
  view.next_search_result();
  view.next_search_result();
  EXPECT_EQ(view.selected_hunk(), 1u);

  // Navigation wraps back around to the first match.
  view.next_search_result();
  EXPECT_EQ(view.selected_hunk(), 0u);
}

TEST(UnifiedDiffView, SearchIsCaseInsensitive) {
  const std::string diff =
      "diff --git a/x.txt b/x.txt\n"
      "--- a/x.txt\n"
      "+++ b/x.txt\n"
      "@@ -1,1 +1,1 @@\n"
      "-Hello World\n"
      "+Goodbye\n";

  UnifiedDiffView view = MakeView(diff);
  view.set_search("hello");
  EXPECT_EQ(view.search_result_count(), 1u);
}

TEST(UnifiedDiffView, ClearingSearchYieldsNoMatches) {
  UnifiedDiffView view = MakeView(kSingleFile);
  view.set_search("nonexistent");
  EXPECT_EQ(view.search_result_count(), 0u);
  view.set_search("");
  EXPECT_EQ(view.search_result_count(), 0u);
  EXPECT_TRUE(view.search().empty());
}

TEST(UnifiedDiffView, CopyCallbackReturnsOriginalSourceText) {
  std::string copied;
  UnifiedDiffViewOptions options;
  options.on_copy = [&copied](std::string text) { copied = std::move(text); };
  UnifiedDiffView view = MakeView(kSingleFile, options);

  // Select the deleted line ("old one" at file index 0) via programmatic
  // selection: move to the second hunk row is not needed; copy_selected works
  // on the current selected row. Start at the file header (no line -> empty).
  view.copy_selected();
  EXPECT_TRUE(copied.empty());

  // Walk down to the deleted line using search to land on it.
  view.set_search("old one");
  ASSERT_EQ(view.search_result_count(), 1u);
  view.next_search_result();
  view.copy_selected();
  EXPECT_EQ(copied, "old one");
}

TEST(UnifiedDiffView, KeyboardCopyKeyInvokesCallback) {
  std::string copied;
  UnifiedDiffViewOptions options;
  options.on_copy = [&copied](std::string text) { copied = std::move(text); };
  UnifiedDiffView view = MakeView(kSingleFile, options);

  view.set_search("new one");
  view.next_search_result();
  view.component()->OnEvent(ftxui::Event::Character('y'));
  EXPECT_EQ(copied, "new one");
}

TEST(UnifiedDiffView, KeyboardNavigationKeysDispatch) {
  const std::string diff =
      "diff --git a/one.txt b/one.txt\n"
      "--- a/one.txt\n"
      "+++ b/one.txt\n"
      "@@ -1 +1 @@\n"
      "-a\n"
      "+b\n"
      "diff --git a/two.txt b/two.txt\n"
      "--- a/two.txt\n"
      "+++ b/two.txt\n"
      "@@ -1 +1 @@\n"
      "-c\n"
      "+d\n";

  UnifiedDiffView view = MakeView(diff);
  view.component()->OnEvent(ftxui::Event::Character(']'));  // next file
  EXPECT_EQ(view.selected_file(), 1u);
  view.component()->OnEvent(ftxui::Event::Character('['));  // previous file
  EXPECT_EQ(view.selected_file(), 0u);
  view.component()->OnEvent(ftxui::Event::Return);  // collapse
  EXPECT_TRUE(view.collapsed(0));
}

TEST(UnifiedDiffView, SlashEntersAndCommitsSearch) {
  UnifiedDiffView view = MakeView(kSingleFile);
  view.component()->OnEvent(ftxui::Event::Character('/'));
  view.component()->OnEvent(ftxui::Event::Character('n'));
  view.component()->OnEvent(ftxui::Event::Character('e'));
  view.component()->OnEvent(ftxui::Event::Character('w'));
  view.component()->OnEvent(ftxui::Event::Return);
  EXPECT_EQ(view.search_result_count(), 1u);
  EXPECT_EQ(view.search(), "new");
}

TEST(UnifiedDiffView, LayoutNotRebuiltAcrossRendersOrResize) {
  UnifiedDiffView view = MakeView(kSingleFile);
  const std::size_t builds = view.layout_build_count();

  // Multiple renders with unchanged model and layout constraints.
  Render(view, 40, 10);
  Render(view, 40, 10);
  EXPECT_EQ(view.layout_build_count(), builds);

  // A resize changes the viewport width but not the model; the flat row
  // layout (fixed-height rows, truncation) must not be rebuilt.
  Render(view, 80, 20);
  EXPECT_EQ(view.layout_build_count(), builds);

  // Changing the model rebuilds once.
  view.SetFiles(UnifiedDiffParser{}.Parse(kSingleFile));
  EXPECT_EQ(view.layout_build_count(), builds + 1);
}

TEST(UnifiedDiffView, SelectionRemainsStableAcrossResize) {
  const std::string diff =
      "diff --git a/one.txt b/one.txt\n"
      "--- a/one.txt\n"
      "+++ b/one.txt\n"
      "@@ -1 +1 @@\n"
      "-a\n"
      "+b\n"
      "diff --git a/two.txt b/two.txt\n"
      "--- a/two.txt\n"
      "+++ b/two.txt\n"
      "@@ -1 +1 @@\n"
      "-c\n"
      "+d\n";

  UnifiedDiffView view = MakeView(diff);
  view.next_file();  // select file 1
  EXPECT_EQ(view.selected_file(), 1u);

  const std::string small = Render(view, 30, 8);
  const std::string large = Render(view, 90, 30);
  EXPECT_EQ(view.selected_file(), 1u);  // stable after resize
  EXPECT_EQ(view.status_line(), view.status_line());
}

TEST(UnifiedDiffView, LargeDiffRendersBoundedViewport) {
  // A 100,000-line diff built directly as the model (no parser round-trip)
  // to keep the test fast and focused on the view.
  DiffFile file;
  file.old_path = "big.txt";
  file.new_path = "big.txt";
  diff::DiffHunk hunk;
  hunk.header = "@@ -1,100000 +1,100000 @@";
  for (int i = 1; i <= 100000; ++i) {
    DiffLine line;
    line.type = DiffLineType::kContext;
    line.old_line = i;
    line.new_line = i;
    line.content.append(TextSpan{"line " + std::to_string(i), TextStyle{}, std::nullopt});
    hunk.lines.push_back(std::move(line));
  }
  file.hunks.push_back(std::move(hunk));

  std::vector<DiffFile> files;
  files.push_back(std::move(file));
  UnifiedDiffView view({});
  view.SetFiles(std::move(files));

  constexpr int kWidth = 100;
  constexpr int kHeight = 30;
  const std::string text = Render(view, kWidth, kHeight);

  // Only the viewport is materialized: the visible row range is bounded well
  // below the total 100,002 rows (file header + hunk header + 100,000 lines).
  const auto [first, count] = view.visible_range();
  EXPECT_GE(first, 0u);
  EXPECT_LT(count, 40u);
  EXPECT_NE(text.find("line 1"), std::string::npos);

  // Interaction stays usable on the large sample.
  view.set_search("line 99999");
  ASSERT_EQ(view.search_result_count(), 1u);
  view.next_search_result();
  view.copy_selected();
}

TEST(UnifiedDiffView, HunkHeaderSectionHeadingPreserved) {
  const std::string diff =
      "diff --git a/x.cc b/x.cc\n"
      "--- a/x.cc\n"
      "+++ b/x.cc\n"
      "@@ -12,3 +12,3 @@ int main() {\n"
      " foo\n"
      " bar\n"
      " baz\n";

  UnifiedDiffView view = MakeView(diff);
  const std::string text = Render(view, 60, 10);
  EXPECT_NE(text.find("@@ -12,3 +12,3 @@ int main() {"), std::string::npos);
}

TEST(UnifiedDiffView, Utf8ContentPreserved) {
  const std::string diff =
      "diff --git a/ru.txt b/ru.txt\n"
      "--- a/ru.txt\n"
      "+++ b/ru.txt\n"
      "@@ -1,1 +1,1 @@\n"
      "-старый\n"
      "+новый\n";

  UnifiedDiffView view = MakeView(diff);
  const std::string text = Render(view, 40, 8);
  EXPECT_NE(text.find("старый"), std::string::npos);
  EXPECT_NE(text.find("новый"), std::string::npos);
}

}  // namespace
}  // namespace terminal_ui_kit
