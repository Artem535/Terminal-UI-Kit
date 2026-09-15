#include <string>
#include <vector>

#include "terminal_ui_kit/core/styled_text.h"
#include "terminal_ui_kit/diff/diff_model.h"
#include "terminal_ui_kit/diff/unified_diff_parser.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace diff {
namespace {

// Concatenates the plain text of every span in a StyledText. Diff lines are
// always emitted as a single span, so this is a simple accessor for tests.
std::string PlainText(const StyledText& text) {
  std::string result;
  for (const TextSpan& span : text.spans()) result += span.text;
  return result;
}

TEST(DiffParser, ParsesSingleFileHunkWithNumbersAndContent) {
  const std::string diff =
      "diff --git a/file.txt b/file.txt\n"
      "index 1111111..2222222 100644\n"
      "--- a/file.txt\n"
      "+++ b/file.txt\n"
      "@@ -1,5 +1,6 @@\n"
      " alpha\n"
      " beta\n"
      "-old one\n"
      "+new one\n"
      "-old two\n"
      "+new two\n"
      " gamma\n";

  const std::vector<DiffFile> files = UnifiedDiffParser{}.Parse(diff);

  ASSERT_EQ(files.size(), 1u);
  const DiffFile& file = files[0];
  EXPECT_EQ(file.old_path, "file.txt");
  EXPECT_EQ(file.new_path, "file.txt");
  ASSERT_EQ(file.hunks.size(), 1u);
  const DiffHunk& hunk = file.hunks[0];
  EXPECT_EQ(hunk.header, "@@ -1,5 +1,6 @@");
  ASSERT_EQ(hunk.lines.size(), 7u);

  EXPECT_EQ(hunk.lines[0].type, DiffLineType::kContext);
  EXPECT_EQ(hunk.lines[0].old_line, 1);
  EXPECT_EQ(hunk.lines[0].new_line, 1);
  EXPECT_EQ(PlainText(hunk.lines[0].content), "alpha");

  EXPECT_EQ(hunk.lines[1].type, DiffLineType::kContext);
  EXPECT_EQ(hunk.lines[1].old_line, 2);
  EXPECT_EQ(hunk.lines[1].new_line, 2);

  EXPECT_EQ(hunk.lines[2].type, DiffLineType::kDeleted);
  EXPECT_EQ(hunk.lines[2].old_line, 3);
  EXPECT_FALSE(hunk.lines[2].new_line.has_value());
  EXPECT_EQ(PlainText(hunk.lines[2].content), "old one");

  EXPECT_EQ(hunk.lines[3].type, DiffLineType::kAdded);
  EXPECT_FALSE(hunk.lines[3].old_line.has_value());
  EXPECT_EQ(hunk.lines[3].new_line, 3);
  EXPECT_EQ(PlainText(hunk.lines[3].content), "new one");

  EXPECT_EQ(hunk.lines[4].type, DiffLineType::kDeleted);
  EXPECT_EQ(hunk.lines[4].old_line, 4);
  EXPECT_EQ(PlainText(hunk.lines[4].content), "old two");

  EXPECT_EQ(hunk.lines[5].type, DiffLineType::kAdded);
  EXPECT_EQ(hunk.lines[5].new_line, 4);
  EXPECT_EQ(PlainText(hunk.lines[5].content), "new two");

  EXPECT_EQ(hunk.lines[6].type, DiffLineType::kContext);
  EXPECT_EQ(hunk.lines[6].old_line, 5);
  EXPECT_EQ(hunk.lines[6].new_line, 5);
  EXPECT_EQ(PlainText(hunk.lines[6].content), "gamma");
}

TEST(DiffParser, ParsesMultipleFiles) {
  const std::string diff =
      "diff --git a/one.txt b/one.txt\n"
      "index 1111111..2222222 100644\n"
      "--- a/one.txt\n"
      "+++ b/one.txt\n"
      "@@ -1 +1 @@\n"
      "-a\n"
      "+b\n"
      "diff --git a/two.txt b/two.txt\n"
      "index 3333333..4444444 100644\n"
      "--- a/two.txt\n"
      "+++ b/two.txt\n"
      "@@ -10,2 +10,3 @@\n"
      " x\n"
      "+extra\n"
      " y\n";

  const std::vector<DiffFile> files = UnifiedDiffParser{}.Parse(diff);

  ASSERT_EQ(files.size(), 2u);
  EXPECT_EQ(files[0].old_path, "one.txt");
  ASSERT_EQ(files[0].hunks.size(), 1u);
  ASSERT_EQ(files[0].hunks[0].lines.size(), 2u);
  EXPECT_EQ(files[0].hunks[0].lines[0].type, DiffLineType::kDeleted);
  EXPECT_EQ(files[0].hunks[0].lines[0].old_line, 1);
  EXPECT_EQ(files[0].hunks[0].lines[1].type, DiffLineType::kAdded);
  EXPECT_EQ(files[0].hunks[0].lines[1].new_line, 1);

  EXPECT_EQ(files[1].old_path, "two.txt");
  ASSERT_EQ(files[1].hunks.size(), 1u);
  const DiffHunk& second = files[1].hunks[0];
  EXPECT_EQ(second.header, "@@ -10,2 +10,3 @@");
  EXPECT_EQ(second.lines[0].old_line, 10);
  EXPECT_EQ(second.lines[0].new_line, 10);
  EXPECT_EQ(second.lines[1].type, DiffLineType::kAdded);
  EXPECT_EQ(second.lines[1].new_line, 11);
  EXPECT_EQ(PlainText(second.lines[1].content), "extra");
}

TEST(DiffParser, ParsesNewFileFromDevNull) {
  const std::string diff =
      "diff --git a/new.txt b/new.txt\n"
      "new file mode 100644\n"
      "index 0000000..abcdef1\n"
      "--- /dev/null\n"
      "+++ b/new.txt\n"
      "@@ -0,0 +1,3 @@\n"
      "+line one\n"
      "+line two\n"
      "+line three\n";

  const std::vector<DiffFile> files = UnifiedDiffParser{}.Parse(diff);

  ASSERT_EQ(files.size(), 1u);
  EXPECT_EQ(files[0].old_path, "/dev/null");
  EXPECT_EQ(files[0].new_path, "new.txt");
  ASSERT_EQ(files[0].hunks.size(), 1u);
  const DiffHunk& hunk = files[0].hunks[0];
  ASSERT_EQ(hunk.lines.size(), 3u);
  for (const DiffLine& line : hunk.lines) {
    EXPECT_EQ(line.type, DiffLineType::kAdded);
    EXPECT_FALSE(line.old_line.has_value());
  }
  EXPECT_EQ(hunk.lines[0].new_line, 1);
  EXPECT_EQ(hunk.lines[1].new_line, 2);
  EXPECT_EQ(hunk.lines[2].new_line, 3);
}

TEST(DiffParser, ParsesDeletedFileToDevNull) {
  const std::string diff =
      "diff --git a/gone.txt b/gone.txt\n"
      "deleted file mode 100644\n"
      "index abcdef1..0000000\n"
      "--- a/gone.txt\n"
      "+++ /dev/null\n"
      "@@ -1,2 +0,0 @@\n"
      "-a\n"
      "-b\n";

  const std::vector<DiffFile> files = UnifiedDiffParser{}.Parse(diff);

  ASSERT_EQ(files.size(), 1u);
  EXPECT_EQ(files[0].old_path, "gone.txt");
  EXPECT_EQ(files[0].new_path, "/dev/null");
  ASSERT_EQ(files[0].hunks.size(), 1u);
  const DiffHunk& hunk = files[0].hunks[0];
  ASSERT_EQ(hunk.lines.size(), 2u);
  for (const DiffLine& line : hunk.lines) {
    EXPECT_EQ(line.type, DiffLineType::kDeleted);
    EXPECT_FALSE(line.new_line.has_value());
  }
  EXPECT_EQ(hunk.lines[0].old_line, 1);
  EXPECT_EQ(hunk.lines[1].old_line, 2);
}

TEST(DiffParser, BinaryFileNoticeYieldsFileWithoutHunks) {
  const std::string diff =
      "diff --git a/img.png b/img.png\n"
      "index 1111111..2222222 100644\n"
      "Binary files a/img.png and b/img.png differ\n";

  const std::vector<DiffFile> files = UnifiedDiffParser{}.Parse(diff);

  ASSERT_EQ(files.size(), 1u);
  EXPECT_EQ(files[0].old_path, "img.png");
  EXPECT_EQ(files[0].new_path, "img.png");
  EXPECT_TRUE(files[0].hunks.empty());
  EXPECT_TRUE(files[0].binary);
}

TEST(DiffParser, DeletedOrAddedLineStartingWithDashesOrPlusesNotMistakenForFileHeader) {
  // A deleted body line whose own text begins with "-- " (e.g. a removed
  // comment "-- x") is written in the diff as "--- x" and must be parsed as a
  // deleted line, not as a "--- " file header (likewise for "+++ " additions).
  const std::string diff =
      "diff --git a/x.txt b/x.txt\n"
      "--- a/x.txt\n"
      "+++ b/x.txt\n"
      "@@ -1,2 +1,2 @@\n"
      " keep\n"
      "--- a removed comment\n"
      "+++ an added thing\n";

  const std::vector<DiffFile> files = UnifiedDiffParser{}.Parse(diff);

  ASSERT_EQ(files.size(), 1u);
  EXPECT_EQ(files[0].old_path, "x.txt");
  EXPECT_EQ(files[0].new_path, "x.txt");
  ASSERT_EQ(files[0].hunks.size(), 1u);
  const DiffHunk& hunk = files[0].hunks[0];
  ASSERT_EQ(hunk.lines.size(), 3u);
  EXPECT_EQ(hunk.lines[0].type, DiffLineType::kContext);
  EXPECT_EQ(PlainText(hunk.lines[0].content), "keep");
  EXPECT_EQ(hunk.lines[1].type, DiffLineType::kDeleted);
  EXPECT_EQ(PlainText(hunk.lines[1].content), "-- a removed comment");
  EXPECT_EQ(hunk.lines[2].type, DiffLineType::kAdded);
  EXPECT_EQ(PlainText(hunk.lines[2].content), "++ an added thing");
}

TEST(DiffParser, IgnoresContentBeforeFirstFileHeader) {
  const std::string diff =
      "arbitrary leading garbage\n"
      "diff --cc combined HEAD\n"
      "diff --git a/real.txt b/real.txt\n"
      "index 1111111..2222222 100644\n"
      "--- a/real.txt\n"
      "+++ b/real.txt\n"
      "@@ -1,1 +1,1 @@\n"
      "-old\n"
      "+new\n";

  const std::vector<DiffFile> files = UnifiedDiffParser{}.Parse(diff);

  ASSERT_EQ(files.size(), 1u);
  // Leading lines that are not `diff --git ` file headers are ignored.
  EXPECT_EQ(files[0].old_path, "real.txt");
  EXPECT_EQ(files[0].hunks.size(), 1u);
}

TEST(DiffParser, MalformedHunkHeaderYieldsEmptyHunk) {
  const std::string diff =
      "diff --git a/x.txt b/x.txt\n"
      "--- a/x.txt\n"
      "+++ b/x.txt\n"
      "@@ some malformed junk @@\n"
      "+not consumed\n";

  const std::vector<DiffFile> files = UnifiedDiffParser{}.Parse(diff);

  ASSERT_EQ(files.size(), 1u);
  ASSERT_EQ(files[0].hunks.size(), 1u);
  EXPECT_EQ(files[0].hunks[0].header, "@@ some malformed junk @@");
  EXPECT_TRUE(files[0].hunks[0].lines.empty());
}

TEST(DiffParser, TruncatedHunkKeepsSeenLines) {
  const std::string diff =
      "diff --git a/x.txt b/x.txt\n"
      "--- a/x.txt\n"
      "+++ b/x.txt\n"
      "@@ -1,10 +1,10 @@\n"
      " one\n"
      "+two\n"
      "-three\n";

  const std::vector<DiffFile> files = UnifiedDiffParser{}.Parse(diff);

  ASSERT_EQ(files.size(), 1u);
  ASSERT_EQ(files[0].hunks.size(), 1u);
  // The hunk declares 10 lines but only 3 are present; the parser keeps them.
  ASSERT_EQ(files[0].hunks[0].lines.size(), 3u);
  EXPECT_EQ(files[0].hunks[0].lines[0].type, DiffLineType::kContext);
  EXPECT_EQ(files[0].hunks[0].lines[1].type, DiffLineType::kAdded);
  EXPECT_EQ(files[0].hunks[0].lines[2].type, DiffLineType::kDeleted);
}

TEST(DiffParser, DropsNoNewlineMarkerLine) {
  // Old side has one context line, new side has that context plus one added
  // line; the trailing "\ No newline at end of file" marker must not be
  // counted as a diff line on either side.
  const std::string diff =
      "diff --git a/x.txt b/x.txt\n"
      "--- a/x.txt\n"
      "+++ b/x.txt\n"
      "@@ -1,1 +1,2 @@\n"
      " keep\n"
      "+add\n"
      "\\ No newline at end of file\n";

  const std::vector<DiffFile> files = UnifiedDiffParser{}.Parse(diff);

  ASSERT_EQ(files.size(), 1u);
  ASSERT_EQ(files[0].hunks.size(), 1u);
  const DiffHunk& hunk = files[0].hunks[0];
  ASSERT_EQ(hunk.lines.size(), 2u);
  EXPECT_EQ(hunk.lines[0].type, DiffLineType::kContext);
  EXPECT_EQ(hunk.lines[0].old_line, 1);
  EXPECT_EQ(hunk.lines[0].new_line, 1);
  EXPECT_EQ(hunk.lines[1].type, DiffLineType::kAdded);
  EXPECT_FALSE(hunk.lines[1].old_line.has_value());
  EXPECT_EQ(hunk.lines[1].new_line, 2);
}

TEST(DiffParser, PreservesUtf8Content) {
  const std::string diff =
      "diff --git a/ru.txt b/ru.txt\n"
      "--- a/ru.txt\n"
      "+++ b/ru.txt\n"
      "@@ -1,3 +1,3 @@\n"
      " привет\n"
      "-старый\n"
      "+новый\n"
      " мир\n";

  const std::vector<DiffFile> files = UnifiedDiffParser{}.Parse(diff);

  ASSERT_EQ(files.size(), 1u);
  const DiffHunk& hunk = files[0].hunks[0];
  ASSERT_EQ(hunk.lines.size(), 4u);
  EXPECT_EQ(hunk.lines[0].type, DiffLineType::kContext);
  EXPECT_EQ(PlainText(hunk.lines[0].content), "привет");
  EXPECT_EQ(hunk.lines[1].type, DiffLineType::kDeleted);
  EXPECT_EQ(PlainText(hunk.lines[1].content), "старый");
  EXPECT_EQ(hunk.lines[2].type, DiffLineType::kAdded);
  EXPECT_EQ(PlainText(hunk.lines[2].content), "новый");
  EXPECT_EQ(hunk.lines[3].type, DiffLineType::kContext);
  EXPECT_EQ(PlainText(hunk.lines[3].content), "мир");
}

TEST(DiffParser, ContextLineContentHasNoLeadingSpace) {
  const std::string diff =
      "diff --git a/x.txt b/x.txt\n"
      "--- a/x.txt\n"
      "+++ b/x.txt\n"
      "@@ -1,1 +1,1 @@\n"
      " indented content\n";

  const std::vector<DiffFile> files = UnifiedDiffParser{}.Parse(diff);

  ASSERT_EQ(files.size(), 1u);
  const DiffLine& line = files[0].hunks[0].lines[0];
  EXPECT_EQ(line.type, DiffLineType::kContext);
  // The leading space is the diff marker and must not appear in content.
  EXPECT_EQ(PlainText(line.content), "indented content");
}

TEST(DiffParser, EmptyInputProducesNoFiles) {
  EXPECT_TRUE(UnifiedDiffParser{}.Parse("").empty());
  EXPECT_TRUE(UnifiedDiffParser{}.Parse("\n\n\n").empty());
}

TEST(DiffParser, HandlesCrLfLineEndings) {
  const std::string diff =
      "diff --git a/x.txt b/x.txt\r\n"
      "--- a/x.txt\r\n"
      "+++ b/x.txt\r\n"
      "@@ -1,1 +1,1 @@\r\n"
      "-old\r\n"
      "+new\r\n";

  const std::vector<DiffFile> files = UnifiedDiffParser{}.Parse(diff);

  ASSERT_EQ(files.size(), 1u);
  const DiffHunk& hunk = files[0].hunks[0];
  ASSERT_EQ(hunk.lines.size(), 2u);
  EXPECT_EQ(PlainText(hunk.lines[0].content), "old");
  EXPECT_EQ(PlainText(hunk.lines[1].content), "new");
}

TEST(DiffParser, UnquotesPathsWithSpaces) {
  const std::string diff =
      "diff --git \"a/my file.txt\" \"b/my file.txt\"\n"
      "index 1111111..2222222 100644\n"
      "--- \"a/my file.txt\"\n"
      "+++ \"b/my file.txt\"\n"
      "@@ -1,1 +1,1 @@\n"
      "-a\n"
      "+b\n";

  const std::vector<DiffFile> files = UnifiedDiffParser{}.Parse(diff);

  ASSERT_EQ(files.size(), 1u);
  EXPECT_EQ(files[0].old_path, "my file.txt");
  EXPECT_EQ(files[0].new_path, "my file.txt");
  EXPECT_EQ(files[0].hunks.size(), 1u);
}

TEST(DiffParser, HunkHeaderPreservesSectionHeading) {
  const std::string diff =
      "diff --git a/x.cc b/x.cc\n"
      "--- a/x.cc\n"
      "+++ b/x.cc\n"
      "@@ -12,3 +12,3 @@ int main() {\n"
      " foo\n"
      " bar\n"
      " baz\n";

  const std::vector<DiffFile> files = UnifiedDiffParser{}.Parse(diff);

  ASSERT_EQ(files.size(), 1u);
  ASSERT_EQ(files[0].hunks.size(), 1u);
  EXPECT_EQ(files[0].hunks[0].header, "@@ -12,3 +12,3 @@ int main() {");
  EXPECT_EQ(files[0].hunks[0].lines.size(), 3u);
  EXPECT_EQ(files[0].hunks[0].lines[0].old_line, 12);
  EXPECT_EQ(files[0].hunks[0].lines[0].new_line, 12);
}

TEST(DiffParser, DoesNotOverRunDeclaredCounts) {
  // Declared new-length is 1 so the second added line must be ignored, as it
  // is beyond the hunk's declared content.
  const std::string diff =
      "diff --git a/x.txt b/x.txt\n"
      "--- a/x.txt\n"
      "+++ b/x.txt\n"
      "@@ -1,1 +1,1 @@\n"
      "+first\n"
      "+second\n";

  const std::vector<DiffFile> files = UnifiedDiffParser{}.Parse(diff);

  ASSERT_EQ(files.size(), 1u);
  ASSERT_EQ(files[0].hunks.size(), 1u);
  ASSERT_EQ(files[0].hunks[0].lines.size(), 1u);
  EXPECT_EQ(PlainText(files[0].hunks[0].lines[0].content), "first");
}

}  // namespace
}  // namespace diff
}  // namespace terminal_ui_kit
