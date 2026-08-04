#include "terminal_ui_kit/diff/unified_diff_parser.h"

#include <string>
#include <string_view>
#include <vector>

#include "terminal_ui_kit/core/styled_text.h"
#include "terminal_ui_kit/diff/diff_model.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace diff {
namespace {

std::string content_of(const StyledText& text) {
  std::string result;
  for (const TextSpan& span : text.spans()) result += span.text;
  return result;
}

TEST(UnifiedDiffParser, EmptyInput) {
  const DiffDocument doc = parse_unified_diff("");
  EXPECT_TRUE(doc.files.empty());
}

TEST(UnifiedDiffParser, SingleFileAdditionsDeletionsContext) {
  const std::string_view input =
      "--- a/foo.txt\n"
      "+++ b/foo.txt\n"
      "@@ -1,4 +1,5 @@\n"
      " first\n"
      "-second\n"
      "+two\n"
      " third\n"
      "+fourth\n";
  const DiffDocument doc = parse_unified_diff(input);
  ASSERT_EQ(doc.files.size(), 1u);

  const DiffFile& file = doc.files[0];
  EXPECT_EQ(file.old_path, "a/foo.txt");
  EXPECT_EQ(file.new_path, "b/foo.txt");
  EXPECT_FALSE(file.is_binary);
  ASSERT_EQ(file.hunks.size(), 1u);

  const DiffHunk& hunk = file.hunks[0];
  EXPECT_EQ(hunk.old_start, 1);
  EXPECT_EQ(hunk.old_count, 4);
  EXPECT_EQ(hunk.new_start, 1);
  EXPECT_EQ(hunk.new_count, 5);
  ASSERT_EQ(hunk.lines.size(), 5u);

  EXPECT_EQ(hunk.lines[0].type, DiffLineType::kContext);
  EXPECT_EQ(hunk.lines[0].old_line, 1);
  EXPECT_EQ(hunk.lines[0].new_line, 1);
  EXPECT_EQ(content_of(hunk.lines[0].content), "first");

  EXPECT_EQ(hunk.lines[1].type, DiffLineType::kDeletion);
  EXPECT_EQ(hunk.lines[1].old_line, 2);
  EXPECT_FALSE(hunk.lines[1].new_line.has_value());
  EXPECT_EQ(content_of(hunk.lines[1].content), "second");

  EXPECT_EQ(hunk.lines[2].type, DiffLineType::kAddition);
  EXPECT_EQ(hunk.lines[2].new_line, 2);
  EXPECT_FALSE(hunk.lines[2].old_line.has_value());
  EXPECT_EQ(content_of(hunk.lines[2].content), "two");

  EXPECT_EQ(hunk.lines[3].type, DiffLineType::kContext);
  EXPECT_EQ(hunk.lines[4].type, DiffLineType::kAddition);
}

TEST(UnifiedDiffParser, LineNumbersTrackAcrossMixedLines) {
  // Deletions advance only the old counter; additions only the new counter;
  // context advances both.
  const std::string_view input =
      "@@ -10,4 +20,4 @@\n"
      " ctx\n"
      "-old11\n"
      "+new20\n"
      "+new21\n"
      " ctx2\n";
  const DiffDocument doc = parse_unified_diff(input);
  ASSERT_EQ(doc.files.size(), 1u);
  const DiffHunk& hunk = doc.files[0].hunks[0];

  EXPECT_EQ(hunk.lines[0].old_line, 10);
  EXPECT_EQ(hunk.lines[0].new_line, 20);

  EXPECT_EQ(hunk.lines[1].type, DiffLineType::kDeletion);
  EXPECT_EQ(hunk.lines[1].old_line, 11);

  // The context line advanced the new counter to 21 before these additions.
  EXPECT_EQ(hunk.lines[2].type, DiffLineType::kAddition);
  EXPECT_EQ(hunk.lines[2].new_line, 21);

  EXPECT_EQ(hunk.lines[3].type, DiffLineType::kAddition);
  EXPECT_EQ(hunk.lines[3].new_line, 22);

  EXPECT_EQ(hunk.lines[4].type, DiffLineType::kContext);
  EXPECT_EQ(hunk.lines[4].old_line, 12);
  EXPECT_EQ(hunk.lines[4].new_line, 23);
}

TEST(UnifiedDiffParser, HunkRangeCountZeroEntries) {
  // "20,0 +0,0" style header describes a pure insertion at line 20.
  const std::string_view input =
      "@@ -20,0 +20,3 @@\n"
      "+one\n"
      "+two\n"
      "+three\n";
  const DiffDocument doc = parse_unified_diff(input);
  ASSERT_EQ(doc.files.size(), 1u);
  const DiffHunk& hunk = doc.files[0].hunks[0];
  EXPECT_EQ(hunk.old_start, 20);
  EXPECT_EQ(hunk.old_count, 0);
  EXPECT_EQ(hunk.new_start, 20);
  EXPECT_EQ(hunk.new_count, 3);
  ASSERT_EQ(hunk.lines.size(), 3u);
  EXPECT_EQ(hunk.lines[0].new_line, 20);
  EXPECT_EQ(hunk.lines[1].new_line, 21);
  EXPECT_EQ(hunk.lines[2].new_line, 22);
}

TEST(UnifiedDiffParser, MultipleFiles) {
  const std::string_view input =
      "diff --git a/one.cc b/one.cc\n"
      "index 0000000..1111111 100644\n"
      "--- a/one.cc\n"
      "+++ b/one.cc\n"
      "@@ -1,2 +1,2 @@\n"
      "-old\n"
      "+new\n"
      "diff --git a/two.h b/two.h\n"
      "index 2222222..3333333 100644\n"
      "--- a/two.h\n"
      "+++ b/two.h\n"
      "@@ -5 +5 @@\n"
      " unchanged\n";
  const DiffDocument doc = parse_unified_diff(input);
  ASSERT_EQ(doc.files.size(), 2u);
  EXPECT_EQ(doc.files[0].old_path, "a/one.cc");
  EXPECT_EQ(doc.files[0].new_path, "b/one.cc");
  EXPECT_EQ(doc.files[1].old_path, "a/two.h");
  EXPECT_EQ(doc.files[1].new_path, "b/two.h");
  ASSERT_EQ(doc.files[1].hunks.size(), 1u);
  // A single value in the header range means a count of 1 (unified rule).
  EXPECT_EQ(doc.files[1].hunks[0].old_count, 1);
  EXPECT_EQ(doc.files[1].hunks[0].new_count, 1);
}

TEST(UnifiedDiffParser, HunkHeaderSectionWithFunctionHeading) {
  const std::string_view input =
      "@@ -1,3 +1,3 @@ int main()\n"
      " a\n"
      " b\n"
      " c\n";
  const DiffDocument doc = parse_unified_diff(input);
  ASSERT_EQ(doc.files.size(), 1u);
  ASSERT_EQ(doc.files[0].hunks.size(), 1u);
  // The raw header, including the trailing function section, is retained.
  EXPECT_EQ(doc.files[0].hunks[0].header, "@@ -1,3 +1,3 @@ int main()");
  EXPECT_EQ(doc.files[0].hunks[0].old_count, 3);
  EXPECT_EQ(doc.files[0].hunks[0].new_count, 3);
}

TEST(UnifiedDiffParser, CrLfLineEndings) {
  const std::string_view input =
      "--- a/win.txt\r\n"
      "+++ b/win.txt\r\n"
      "@@ -1,2 +1,2 @@\r\n"
      "-old\r\n"
      "+new\r\n"
      " same\r\n";
  const DiffDocument doc = parse_unified_diff(input);
  ASSERT_EQ(doc.files.size(), 1u);
  ASSERT_EQ(doc.files[0].hunks.size(), 1u);
  const DiffHunk& hunk = doc.files[0].hunks[0];
  ASSERT_EQ(hunk.lines.size(), 3u);
  EXPECT_EQ(hunk.lines[0].type, DiffLineType::kDeletion);
  EXPECT_EQ(content_of(hunk.lines[0].content), "old");
  EXPECT_EQ(hunk.lines[1].type, DiffLineType::kAddition);
  EXPECT_EQ(content_of(hunk.lines[1].content), "new");
  EXPECT_EQ(hunk.lines[2].type, DiffLineType::kContext);
}

TEST(UnifiedDiffParser, NoNewlineMarker) {
  const std::string_view input =
      "@@ -1,2 +1,2 @@\n"
      "-line\n"
      "\\ No newline at end of file\n"
      "+line\n";
  const DiffDocument doc = parse_unified_diff(input);
  ASSERT_EQ(doc.files.size(), 1u);
  const DiffHunk& hunk = doc.files[0].hunks[0];
  ASSERT_EQ(hunk.lines.size(), 3u);
  EXPECT_EQ(hunk.lines[1].type, DiffLineType::kNoNewline);
  // The line is written as "\ No newline at end of file"; the backslash is
  // the marker, so the leading space remains part of the displayed text.
  EXPECT_EQ(content_of(hunk.lines[1].content), " No newline at end of file");
  // The marker does not consume a line number; the following addition still
  // lands on the same new-file line as the deleted line it replaces.
  EXPECT_EQ(hunk.lines[2].type, DiffLineType::kAddition);
}

TEST(UnifiedDiffParser, GitPathHeaderWithTimestampSuffix) {
  const std::string_view input =
      "--- a/foo.txt\t2026-01-01 10:00:00 +0000\n"
      "+++ b/foo.txt\t2026-01-01 10:00:01 +0000\n"
      "@@ -1 +1 @@\n"
      " x\n";
  const DiffDocument doc = parse_unified_diff(input);
  ASSERT_EQ(doc.files.size(), 1u);
  EXPECT_EQ(doc.files[0].old_path, "a/foo.txt");
  EXPECT_EQ(doc.files[0].new_path, "b/foo.txt");
}

TEST(UnifiedDiffParser, BinaryFilesDiffer) {
  const std::string_view input =
      "diff --git a/img.png b/img.png\n"
      "index 0000000..1111111 100644\n"
      "Binary files a/img.png and b/img.png differ\n";
  const DiffDocument doc = parse_unified_diff(input);
  ASSERT_EQ(doc.files.size(), 1u);
  EXPECT_TRUE(doc.files[0].is_binary);
  EXPECT_EQ(doc.files[0].old_path, "a/img.png");
  EXPECT_EQ(doc.files[0].new_path, "b/img.png");
  EXPECT_TRUE(doc.files[0].hunks.empty());
}

TEST(UnifiedDiffParser, GitBinaryPatchNotice) {
  const std::string_view input =
      "diff --git a/f.bin b/f.bin\n"
      "index 0000000..1111111 100644\n"
      "GIT binary patch\n";
  const DiffDocument doc = parse_unified_diff(input);
  ASSERT_EQ(doc.files.size(), 1u);
  EXPECT_TRUE(doc.files[0].is_binary);
}

TEST(UnifiedDiffParser, NonGitFileHeader) {
  // A plain "diff -u" style header with no "diff --git" line still yields a
  // single file with both paths from --- / +++.
  const std::string_view input =
      "--- old.txt\n"
      "+++ new.txt\n"
      "@@ -1,1 +1,1 @@\n"
      " x\n";
  const DiffDocument doc = parse_unified_diff(input);
  ASSERT_EQ(doc.files.size(), 1u);
  EXPECT_EQ(doc.files[0].old_path, "old.txt");
  EXPECT_EQ(doc.files[0].new_path, "new.txt");
}

TEST(UnifiedDiffParser, HeaderOnlyNoHunks) {
  const std::string_view input =
      "diff --git a/empty.txt b/empty.txt\n"
      "--- a/empty.txt\n"
      "+++ b/empty.txt\n";
  const DiffDocument doc = parse_unified_diff(input);
  ASSERT_EQ(doc.files.size(), 1u);
  EXPECT_TRUE(doc.files[0].hunks.empty());
}

TEST(UnifiedDiffParser, MalformedHunkHeaderSkipped) {
  const std::string_view input =
      "@@ not-a-range @@\n"
      "@@ -1,2 +1,2 @@\n"
      " a\n"
      " b\n";
  const DiffDocument doc = parse_unified_diff(input);
  ASSERT_EQ(doc.files.size(), 1u);
  ASSERT_EQ(doc.files[0].hunks.size(), 1u);
  EXPECT_EQ(doc.files[0].hunks[0].old_start, 1);
}

TEST(UnifiedDiffParser, TrailingContentWithoutHunkIgnored) {
  // Lines that look like content but appear after the last hunk (or before
  // any hunk) are not attached to any hunk and are ignored.
  const std::string_view input =
      "--- a/x\n"
      "+++ b/x\n"
      " context\n";
  const DiffDocument doc = parse_unified_diff(input);
  ASSERT_EQ(doc.files.size(), 1u);
  EXPECT_TRUE(doc.files[0].hunks.empty());
}

TEST(UnifiedDiffParser, MetadataLinesBetweenHunksDoNotBreakParsing) {
  const std::string_view input =
      "@@ -1,1 +1,1 @@\n"
      " a\n"
      "@@ -2,1 +2,1 @@\n"
      " b\n";
  const DiffDocument doc = parse_unified_diff(input);
  ASSERT_EQ(doc.files.size(), 1u);
  ASSERT_EQ(doc.files[0].hunks.size(), 2u);
  ASSERT_EQ(doc.files[0].hunks[1].lines.size(), 1u);
  EXPECT_EQ(doc.files[0].hunks[1].lines[0].new_line, 2);
}

TEST(UnifiedDiffParser, DiffInsideContentStartsNoFileByItself) {
  // A "diff --git" appearing mid-file while a hunk is open terminates the
  // hunk; without a following header it does not create a new file.
  const std::string_view input =
      "@@ -1 +1 @@\n"
      " x\n"
      "diff --git a/p b/p\n";
  const DiffDocument doc = parse_unified_diff(input);
  ASSERT_EQ(doc.files.size(), 2u);
  EXPECT_EQ(doc.files[0].hunks.size(), 1u);
  EXPECT_TRUE(doc.files[1].hunks.empty());
  EXPECT_EQ(doc.files[1].old_path, "a/p");
  EXPECT_EQ(doc.files[1].new_path, "b/p");
}

TEST(UnifiedDiffParser, QuotedGitPathsWithSpaces) {
  const std::string_view input =
      "diff --git \"a/we ird.png\" \"b/we ird.png\"\n"
      "index 0000000..1111111 100644\n"
      "Binary files a/we ird.png and b/we ird.png differ\n";
  const DiffDocument doc = parse_unified_diff(input);
  ASSERT_EQ(doc.files.size(), 1u);
  EXPECT_EQ(doc.files[0].old_path, "a/we ird.png");
  EXPECT_EQ(doc.files[0].new_path, "b/we ird.png");
  EXPECT_TRUE(doc.files[0].is_binary);
}

TEST(UnifiedDiffParser, StandaloneBinaryFilesDifferPathParsing) {
  const std::string_view input =
      "diff --git a/x.png b/x.png\n"
      "index 0000000..1111111 100644\n"
      "Binary files a/x.png and b/x.png differ\n";
  const DiffDocument doc = parse_unified_diff(input);
  ASSERT_EQ(doc.files.size(), 1u);
  EXPECT_EQ(doc.files[0].old_path, "a/x.png");
  EXPECT_EQ(doc.files[0].new_path, "b/x.png");
  EXPECT_TRUE(doc.files[0].is_binary);
}

TEST(UnifiedDiffParser, RenameWithNoContentLines) {
  // A pure rename carries no hunks; the paths still come from diff --git.
  const std::string_view input =
      "diff --git a/old.txt b/new.txt\n"
      "similarity index 100%\n"
      "rename from old.txt\n"
      "rename to new.txt\n";
  const DiffDocument doc = parse_unified_diff(input);
  ASSERT_EQ(doc.files.size(), 1u);
  EXPECT_EQ(doc.files[0].old_path, "a/old.txt");
  EXPECT_EQ(doc.files[0].new_path, "b/new.txt");
  EXPECT_TRUE(doc.files[0].hunks.empty());
}

}  // namespace
}  // namespace diff
}  // namespace terminal_ui_kit
