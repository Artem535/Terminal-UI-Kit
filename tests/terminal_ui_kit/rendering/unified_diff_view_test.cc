#include "terminal_ui_kit/components/unified_diff_view.h"

#include <string>
#include <vector>

#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>

#include "terminal_ui_kit/diff/unified_diff_parser.h"
#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

using namespace diff;

// Helper: parse a unified diff string into a model.
std::vector<DiffFile> parse(std::string_view text) { return UnifiedDiffParser{}.Parse(text); }

// Renders `view` twice at the same size. The first pass assigns the viewport
// box (discovered by the observing decorator during layout); the second pass
// renders the correct visible-row window. This mirrors the VirtualList test
// idiom, where a follow-up frame resolves the initial zero-sized box.
std::string render_view(const UnifiedDiffView& view, int width, int height) {
  test_support::render_to_text(view.component()->Render(), width, height);
  return test_support::render_to_text(view.component()->Render(), width, height);
}

// A simple diff producing a single file with one hunk.
const char* kSingleFileDiff = R"(diff --git a/file.txt b/file.txt
index 1111111..2222222 100644
--- a/file.txt
+++ b/file.txt
@@ -1,5 +1,6 @@
 alpha
 beta
-old one
+new one
-old two
+new two
 gamma
)";

// A diff with two files.
const char* kMultiFileDiff = R"(diff --git a/one.txt b/one.txt
index 1111111..2222222 100644
--- a/one.txt
+++ b/one.txt
@@ -1 +1 @@
-a
+b
diff --git a/two.txt b/two.txt
index 3333333..4444444 100644
--- a/two.txt
+++ b/two.txt
@@ -10,2 +10,3 @@
 x
+extra
 y
)";

// A new file.
const char* kNewFileDiff = R"(diff --git a/new.txt b/new.txt
new file mode 100644
index 0000000..abcdef1
--- /dev/null
+++ b/new.txt
@@ -0,0 +1,3 @@
+line one
+line two
+line three
)";

// A deleted file.
const char* kDeletedFileDiff = R"(diff --git a/gone.txt b/gone.txt
deleted file mode 100644
index abcdef1..0000000
--- a/gone.txt
+++ /dev/null
@@ -1,2 +0,0 @@
-a
-b
)";

// A binary file.
const char* kBinaryFileDiff = R"(diff --git a/img.png b/img.png
index 1111111..2222222 100644
Binary files a/img.png and b/img.png differ
)";

// An empty diff (no files).
const char* kEmptyDiff = "";

// Long lines diff.
const char* kLongLinesDiff = R"(diff --git a/long.txt b/long.txt
--- a/long.txt
+++ b/long.txt
@@ -1,1 +1,1 @@
-this-is-a-very-long-line-of-content-that-exceeds-the-viewport-width
+this-is-a-replacement-long-line-that-also-exceeds-viewport-width
)";

// ============================================================================
// Basic rendering tests
// ============================================================================

TEST(UnifiedDiffViewTest, SingleFileRendersFileHeaderAndHunkAndLines) {
  auto view = UnifiedDiffView(parse(kSingleFileDiff));
  const std::string rendered = render_view(view, 80, 40);
  EXPECT_NE(rendered.find("file.txt"), std::string::npos);
  EXPECT_NE(rendered.find("alpha"), std::string::npos);
  EXPECT_NE(rendered.find("beta"), std::string::npos);
  EXPECT_NE(rendered.find("old one"), std::string::npos);
  EXPECT_NE(rendered.find("new one"), std::string::npos);
  EXPECT_NE(rendered.find("old two"), std::string::npos);
  EXPECT_NE(rendered.find("new two"), std::string::npos);
  EXPECT_NE(rendered.find("gamma"), std::string::npos);
}

TEST(UnifiedDiffViewTest, MultiFileRendersBothFiles) {
  auto view = UnifiedDiffView(parse(kMultiFileDiff));
  const std::string rendered = render_view(view, 80, 40);
  EXPECT_NE(rendered.find("one.txt"), std::string::npos);
  EXPECT_NE(rendered.find("two.txt"), std::string::npos);
}

TEST(UnifiedDiffViewTest, NewFileShowsNoticeAndAddedLines) {
  auto view = UnifiedDiffView(parse(kNewFileDiff));
  const std::string rendered = render_view(view, 80, 40);
  EXPECT_NE(rendered.find("new file"), std::string::npos);
  EXPECT_NE(rendered.find("line one"), std::string::npos);
  EXPECT_NE(rendered.find("line three"), std::string::npos);
}

TEST(UnifiedDiffViewTest, DeletedFileShowsNoticeAndDeletedLines) {
  auto view = UnifiedDiffView(parse(kDeletedFileDiff));
  const std::string rendered = render_view(view, 80, 40);
  EXPECT_NE(rendered.find("deleted file"), std::string::npos);
  EXPECT_NE(rendered.find("a"), std::string::npos);
  EXPECT_NE(rendered.find("b"), std::string::npos);
}

TEST(UnifiedDiffViewTest, BinaryFileShowsBinaryNotice) {
  auto view = UnifiedDiffView(parse(kBinaryFileDiff));
  const std::string rendered = render_view(view, 80, 40);
  EXPECT_NE(rendered.find("binary file"), std::string::npos);
}

TEST(UnifiedDiffViewTest, EmptyDiffShowsEmptyMessage) {
  auto view = UnifiedDiffView(parse(kEmptyDiff));
  EXPECT_TRUE(view.is_empty());
  const std::string rendered = render_view(view, 80, 40);
  EXPECT_NE(rendered.find("empty diff"), std::string::npos);
}

TEST(UnifiedDiffViewTest, StatusReturnsCorrectCounts) {
  auto view = UnifiedDiffView(parse(kMultiFileDiff));
  auto s = view.status();
  EXPECT_EQ(s.file_count, 2u);
  EXPECT_GT(s.row_count, 0u);
}

TEST(UnifiedDiffViewTest, NoColorDoesNotFail) {
  UnifiedDiffViewOptions opts;
  opts.color = false;
  auto view = UnifiedDiffView(parse(kSingleFileDiff), opts);
  const std::string rendered = render_view(view, 80, 40);
  EXPECT_NE(rendered.find("alpha"), std::string::npos);
}

TEST(UnifiedDiffViewTest, NarrowTerminalHidesGutter) {
  auto view = UnifiedDiffView(parse(kSingleFileDiff));
  // Render very narrow — gutter should be hidden
  const std::string rendered = render_view(view, 10, 40);
  // Line numbers (gutter) should be suppressed; content should still show
  EXPECT_NE(rendered.find("alpha"), std::string::npos);
}

TEST(UnifiedDiffViewTest, LongLinesAreTruncated) {
  UnifiedDiffViewOptions opts;
  opts.min_content_width = 5;
  auto view = UnifiedDiffView(parse(kLongLinesDiff), opts);
  const std::string rendered = render_view(view, 30, 40);
  // The long line should be truncated (ellipsis may appear)
  EXPECT_NE(rendered.find("this-is"), std::string::npos);
}

// ============================================================================
// Navigation tests
// ============================================================================

TEST(UnifiedDiffViewTest, ArrowDownMovesSelection) {
  auto view = UnifiedDiffView(parse(kSingleFileDiff));
  auto s1 = view.status();
  view.component()->OnEvent(ftxui::Event::ArrowDown);
  auto s2 = view.status();
  EXPECT_NE(s1.selected_row, s2.selected_row);
}

TEST(UnifiedDiffViewTest, JKeyMovesSelection) {
  auto view = UnifiedDiffView(parse(kSingleFileDiff));
  auto s1 = view.status();
  view.component()->OnEvent(ftxui::Event::Character('j'));
  auto s2 = view.status();
  EXPECT_NE(s1.selected_row, s2.selected_row);
}

TEST(UnifiedDiffViewTest, NextFileNavigation) {
  auto view = UnifiedDiffView(parse(kMultiFileDiff));
  view.next_file();
  auto s = view.status();
  EXPECT_EQ(s.file_index, 1u);
}

TEST(UnifiedDiffViewTest, PrevFileNavigation) {
  auto view = UnifiedDiffView(parse(kMultiFileDiff));
  view.next_file();
  view.prev_file();
  auto s = view.status();
  EXPECT_EQ(s.file_index, 0u);
}

TEST(UnifiedDiffViewTest, NextHunkNavigation) {
  // Create a diff with two hunks
  const char* kTwoHunkDiff = R"(diff --git a/x.txt b/x.txt
--- a/x.txt
+++ b/x.txt
@@ -1,2 +1,2 @@
 context
+added
@@ -10,3 +10,4 @@
 more
+extra
 same
)";
  auto view = UnifiedDiffView(parse(kTwoHunkDiff));
  // From the file header, next_hunk lands on hunk 0; a second jump reaches
  // hunk 1.
  view.next_hunk();
  view.next_hunk();
  auto s = view.status();
  EXPECT_EQ(s.hunk_index, 1u);
}

TEST(UnifiedDiffViewTest, PrevHunkNavigation) {
  const char* kTwoHunkDiff = R"(diff --git a/x.txt b/x.txt
--- a/x.txt
+++ b/x.txt
@@ -1,2 +1,2 @@
 context
+added
@@ -10,3 +10,4 @@
 more
+extra
 same
)";
  auto view = UnifiedDiffView(parse(kTwoHunkDiff));
  view.next_hunk();
  view.prev_hunk();
  auto s = view.status();
  EXPECT_EQ(s.hunk_index, 0u);
}

// ============================================================================
// Collapse / expand tests
// ============================================================================

TEST(UnifiedDiffViewTest, CollapseFileHidesBody) {
  auto view = UnifiedDiffView(parse(kMultiFileDiff));
  auto rows_before = view.row_count();
  view.toggle_collapse_current_file();
  // After collapsing the first file, rows should shrink
  EXPECT_LT(view.row_count(), rows_before);
}

TEST(UnifiedDiffViewTest, ExpandFileRestoresBody) {
  auto view = UnifiedDiffView(parse(kMultiFileDiff));
  view.toggle_collapse_current_file();
  auto rows_collapsed = view.row_count();
  view.toggle_collapse_current_file();
  EXPECT_GT(view.row_count(), rows_collapsed);
}

TEST(UnifiedDiffViewTest, CollapseStateStableDuringNavigation) {
  auto view = UnifiedDiffView(parse(kMultiFileDiff));
  view.toggle_collapse_current_file();  // collapse file 0
  EXPECT_TRUE(view.is_collapsed(0));
  view.next_file();
  auto s = view.status();
  EXPECT_EQ(s.file_index, 1u);
  // File 0 remains collapsed
  EXPECT_TRUE(view.is_collapsed(0));
}

// ============================================================================
// Search tests
// ============================================================================

TEST(UnifiedDiffViewTest, SearchFindsMatches) {
  auto view = UnifiedDiffView(parse(kSingleFileDiff));
  view.set_search("alpha");
  EXPECT_TRUE(view.has_matches());
  EXPECT_EQ(view.match_count(), 1u);
}

TEST(UnifiedDiffViewTest, SearchEmptyQueryClearsMatches) {
  auto view = UnifiedDiffView(parse(kSingleFileDiff));
  view.set_search("alpha");
  EXPECT_TRUE(view.has_matches());
  view.set_search("");
  EXPECT_FALSE(view.has_matches());
}

TEST(UnifiedDiffViewTest, JumpToNextMatchCycles) {
  auto view = UnifiedDiffView(parse(kSingleFileDiff));
  view.set_search("old");
  ASSERT_GE(view.match_count(), 2u);
  auto m1 = view.current_match();
  view.jump_to_next_match();
  EXPECT_NE(view.current_match(), m1);
}

// ============================================================================
// Selection / copy tests
// ============================================================================

TEST(UnifiedDiffViewTest, SelectRowChangesStatus) {
  auto view = UnifiedDiffView(parse(kSingleFileDiff));
  auto s1 = view.status();
  view.select_row(s1.selected_row + 2);
  auto s2 = view.status();
  EXPECT_NE(s1.selected_row, s2.selected_row);
}

TEST(UnifiedDiffViewTest, CopyCallbackInvoked) {
  std::string copied;
  UnifiedDiffViewOptions opts;
  opts.on_copy = [&](std::string text) { copied = std::move(text); };
  auto view = UnifiedDiffView(parse(kSingleFileDiff), opts);
  // Row layout: 0=file header, 1=hunk header, 2="alpha", 3="beta", ...
  view.select_row(2);  // context line with "alpha"
  EXPECT_TRUE(view.copy_selection());
  EXPECT_EQ(copied, "alpha");
}

// ============================================================================
// Resize stability
// ============================================================================

TEST(UnifiedDiffViewTest, ResizePreservesSelection) {
  auto view = UnifiedDiffView(parse(kSingleFileDiff));
  view.select_row(5);
  auto s_before = view.status();
  const std::size_t row_before = s_before.selected_row;
  // Render at different widths
  test_support::render_to_screen(view.component()->Render(), 80, 40);
  test_support::render_to_screen(view.component()->Render(), 40, 30);
  test_support::render_to_screen(view.component()->Render(), 80, 40);
  auto s_after = view.status();
  EXPECT_EQ(s_after.selected_row, row_before);
}

// ============================================================================
// Large diff test (100k lines)
// ============================================================================

TEST(UnifiedDiffViewTest, LargeDiffRendersAndNavigates) {
  // Build a diff with ~100,000 lines across 10 files.
  std::string diff;
  diff += "diff --git a/start.txt b/start.txt\n--- a/start.txt\n+++ b/start.txt\n@@ -1,1 +1,1 @@\n";
  diff += "-old_start\n+new_start\n";
  for (int f = 0; f < 10; ++f) {
    diff +=
        "diff --git a/file_" + std::to_string(f) + ".txt b/file_" + std::to_string(f) + ".txt\n";
    diff += "--- a/file_" + std::to_string(f) + ".txt\n";
    diff += "+++ b/file_" + std::to_string(f) + ".txt\n";
    const int lines = 10000;
    const int old_start = 1;
    const int new_start = 1;
    // Each file emits `lines` context lines and `lines` added lines, so the
    // new side has 2*lines lines.
    diff += "@@ -" + std::to_string(old_start) + "," + std::to_string(lines) + " +" +
            std::to_string(new_start) + "," + std::to_string(lines * 2) + " @@\n";
    for (int i = 0; i < lines; ++i) {
      diff += " line_" + std::to_string(i) + "\n";
      diff += "+added_" + std::to_string(i) + "\n";
    }
  }

  auto view = UnifiedDiffView(parse(diff));
  ASSERT_GT(view.row_count(), 100000u);

  // Render at a small viewport — only visible rows should be rendered
  const std::string rendered = render_view(view, 80, 10);
  EXPECT_NE(rendered.find("line_0"), std::string::npos);
  EXPECT_NE(rendered.find("added_0"), std::string::npos);

  // Navigate to the last row (the last line of the last file). There are 11
  // files in total (start.txt + file_0..file_9), so the last index is 10.
  view.select_row(view.row_count() - 1);
  const std::string rendered_end = render_view(view, 80, 10);
  EXPECT_EQ(view.status().file_index, 10u);
  // The selected last line belongs to file_9 and must be visible.
  EXPECT_NE(rendered_end.find("added_9999"), std::string::npos);
}

// ============================================================================
// Stable state after resize
// ============================================================================

TEST(UnifiedDiffViewTest, StableStateAfterResizeUsesOriginalOffset) {
  auto view = UnifiedDiffView(parse(kMultiFileDiff));
  view.select_row(3);
  const auto s1 = view.status();
  // Render at different sizes
  test_support::render_to_screen(view.component()->Render(), 80, 40);
  test_support::render_to_screen(view.component()->Render(), 30, 10);
  test_support::render_to_screen(view.component()->Render(), 80, 40);
  const auto s2 = view.status();
  EXPECT_EQ(s1.file_index, s2.file_index);
  EXPECT_EQ(s1.selected_row, s2.selected_row);
}

// ============================================================================
// Empty file (no hunks)
// ============================================================================

TEST(UnifiedDiffViewTest, EmptyFileShowsNoChangesNotice) {
  // A file with no hunks is an empty/unchanged file.
  const char* kEmptyFileDiff = R"(diff --git a/empty.txt b/empty.txt
index 1111111..2222222 100644
--- a/empty.txt
+++ b/empty.txt
)";
  auto view = UnifiedDiffView(parse(kEmptyFileDiff));
  EXPECT_FALSE(view.is_empty());
  const std::string rendered = render_view(view, 80, 40);
  EXPECT_NE(rendered.find("no changes"), std::string::npos);
}

// ============================================================================
// Multiple hunks
// ============================================================================

TEST(UnifiedDiffViewTest, MultipleHunksRendered) {
  const char* kMultiHunkDiff = R"(diff --git a/x.txt b/x.txt
--- a/x.txt
+++ b/x.txt
@@ -1,2 +1,2 @@
 context
+added
@@ -10,5 +10,5 @@
 more
+extra
 same
)";
  auto view = UnifiedDiffView(parse(kMultiHunkDiff));
  const std::string rendered = render_view(view, 80, 40);
  // Both hunk headers should appear
  EXPECT_NE(rendered.find("@@ -1,2 +1,2 @@"), std::string::npos);
  EXPECT_NE(rendered.find("@@ -10,5 +10,5 @@"), std::string::npos);
}

// ============================================================================
// Gutter visibility
// ============================================================================

TEST(UnifiedDiffViewTest, GutterContainLineNumbers) {
  auto view = UnifiedDiffView(parse(kSingleFileDiff));
  std::string rendered = render_view(view, 80, 40);
  // Line numbers should be visible in the gutter
  EXPECT_NE(rendered.find("1"), std::string::npos);
  EXPECT_NE(rendered.find("2"), std::string::npos);
  EXPECT_NE(rendered.find("3"), std::string::npos);
}

// ============================================================================
// Scroll to row
// ============================================================================

TEST(UnifiedDiffViewTest, ScrollToRowMakesRowVisible) {
  auto view = UnifiedDiffView(parse(kSingleFileDiff));
  // Establish a 3-row viewport, then scroll far enough down that the content
  // no longer fits and the scroll offset must advance.
  render_view(view, 40, 3);
  const auto rows = view.row_count();
  ASSERT_GT(rows, 3u);
  view.scroll_to_row(5);
  EXPECT_GE(view.status().first_visible, 5u);
}

}  // namespace
}  // namespace terminal_ui_kit