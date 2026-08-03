#include <string>

#include "terminal_ui_kit/diff/unified_diff_parser.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

// --- basic success cases ---

TEST(DiffParser, EmptyInputReturnsNoFiles) {
  const auto result = UnifiedDiffParser::Parse("");
  EXPECT_TRUE(result.success);
  EXPECT_TRUE(result.error_message.empty());
  EXPECT_TRUE(result.files.empty());
}

TEST(DiffParser, SimpleSingleFileDiff) {
  const std::string diff = R"(diff --git a/hello.txt b/hello.txt
index 1234567..abcdefg 100644
--- a/hello.txt
+++ b/hello.txt
@@ -1,3 +1,3 @@
 line one
-line two
+line two modified
 line three
)";
  const auto result = UnifiedDiffParser::Parse(diff);
  EXPECT_TRUE(result.success);
  ASSERT_EQ(result.files.size(), 1u);

  const DiffFile& file = result.files[0];
  EXPECT_EQ(file.old_path, "hello.txt");
  EXPECT_EQ(file.new_path, "hello.txt");
  EXPECT_FALSE(file.is_binary);
  EXPECT_FALSE(file.old_no_newline);
  EXPECT_FALSE(file.new_no_newline);
  ASSERT_EQ(file.hunks.size(), 1u);

  const DiffHunk& hunk = file.hunks[0];
  EXPECT_EQ(hunk.old_start, 1);
  EXPECT_EQ(hunk.old_count, 3);
  EXPECT_EQ(hunk.new_start, 1);
  EXPECT_EQ(hunk.new_count, 3);
  ASSERT_EQ(hunk.lines.size(), 4u);

  EXPECT_EQ(hunk.lines[0].type, DiffLineType::kContext);
  EXPECT_EQ(hunk.lines[0].text, "line one");
  EXPECT_EQ(hunk.lines[0].old_line, 1);
  EXPECT_EQ(hunk.lines[0].new_line, 1);

  EXPECT_EQ(hunk.lines[1].type, DiffLineType::kRemoved);
  EXPECT_EQ(hunk.lines[1].text, "line two");
  EXPECT_EQ(hunk.lines[1].old_line, 2);
  EXPECT_EQ(hunk.lines[1].new_line, std::nullopt);

  EXPECT_EQ(hunk.lines[2].type, DiffLineType::kAdded);
  EXPECT_EQ(hunk.lines[2].text, "line two modified");
  EXPECT_EQ(hunk.lines[2].old_line, std::nullopt);
  EXPECT_EQ(hunk.lines[2].new_line, 2);

  EXPECT_EQ(hunk.lines[3].type, DiffLineType::kContext);
  EXPECT_EQ(hunk.lines[3].text, "line three");
  EXPECT_EQ(hunk.lines[3].old_line, 3);
  EXPECT_EQ(hunk.lines[3].new_line, 3);
}

TEST(DiffParser, NewFileDiff) {
  const std::string diff = R"(diff --git a/new_file.txt b/new_file.txt
new file mode 100644
index 0000000..1234567
--- /dev/null
+++ b/new_file.txt
@@ -0,0 +1,2 @@
+hello
+world
)";
  const auto result = UnifiedDiffParser::Parse(diff);
  EXPECT_TRUE(result.success);
  ASSERT_EQ(result.files.size(), 1u);

  const DiffFile& file = result.files[0];
  EXPECT_EQ(file.old_path, "/dev/null");
  EXPECT_EQ(file.new_path, "new_file.txt");
  ASSERT_EQ(file.hunks.size(), 1u);

  const DiffHunk& hunk = file.hunks[0];
  EXPECT_EQ(hunk.old_start, 0);
  EXPECT_EQ(hunk.old_count, 0);
  EXPECT_EQ(hunk.new_start, 1);
  EXPECT_EQ(hunk.new_count, 2);
  ASSERT_EQ(hunk.lines.size(), 2u);

  EXPECT_EQ(hunk.lines[0].type, DiffLineType::kAdded);
  EXPECT_EQ(hunk.lines[0].text, "hello");
  EXPECT_EQ(hunk.lines[0].new_line, 1);

  EXPECT_EQ(hunk.lines[1].type, DiffLineType::kAdded);
  EXPECT_EQ(hunk.lines[1].text, "world");
  EXPECT_EQ(hunk.lines[1].new_line, 2);
}

TEST(DiffParser, DeletedFileDiff) {
  const std::string diff = R"(diff --git a/old_file.txt b/old_file.txt
deleted file mode 100644
index 1234567..0000000
--- a/old_file.txt
+++ /dev/null
@@ -1,2 +0,0 @@
-goodbye
-old
)";
  const auto result = UnifiedDiffParser::Parse(diff);
  EXPECT_TRUE(result.success);
  ASSERT_EQ(result.files.size(), 1u);

  const DiffFile& file = result.files[0];
  EXPECT_EQ(file.old_path, "old_file.txt");
  EXPECT_EQ(file.new_path, "/dev/null");
  ASSERT_EQ(file.hunks.size(), 1u);

  const DiffHunk& hunk = file.hunks[0];
  EXPECT_EQ(hunk.old_start, 1);
  EXPECT_EQ(hunk.old_count, 2);
  EXPECT_EQ(hunk.new_start, 0);
  EXPECT_EQ(hunk.new_count, 0);
  ASSERT_EQ(hunk.lines.size(), 2u);

  EXPECT_EQ(hunk.lines[0].type, DiffLineType::kRemoved);
  EXPECT_EQ(hunk.lines[0].text, "goodbye");
  EXPECT_EQ(hunk.lines[0].old_line, 1);

  EXPECT_EQ(hunk.lines[1].type, DiffLineType::kRemoved);
  EXPECT_EQ(hunk.lines[1].text, "old");
  EXPECT_EQ(hunk.lines[1].old_line, 2);
}

TEST(DiffParser, BinaryFileDiff) {
  const std::string diff =
      "diff --git a/image.png b/image.png\n"
      "index 1234567..abcdefg 100644\n"
      "Binary files a/image.png and b/image.png differ\n";
  const auto result = UnifiedDiffParser::Parse(diff);
  EXPECT_TRUE(result.success);
  ASSERT_EQ(result.files.size(), 1u);

  const DiffFile& file = result.files[0];
  EXPECT_TRUE(file.is_binary);
  EXPECT_TRUE(file.hunks.empty());
  // Paths are extracted from the diff --git header.
  EXPECT_EQ(file.old_path, "image.png");
  EXPECT_EQ(file.new_path, "image.png");
}

TEST(DiffParser, CrlfLineEndings) {
  const std::string diff =
      "diff --git a/file.txt b/file.txt\r\n"
      "--- a/file.txt\r\n"
      "+++ b/file.txt\r\n"
      "@@ -1,2 +1,2 @@\r\n"
      " line1\r\n"
      "-line2\r\n"
      "+two\r\n";
  const auto result = UnifiedDiffParser::Parse(diff);
  EXPECT_TRUE(result.success);
  ASSERT_EQ(result.files.size(), 1u);
  EXPECT_EQ(result.files[0].hunks.size(), 1u);
  ASSERT_EQ(result.files[0].hunks[0].lines.size(), 3u);
  EXPECT_EQ(result.files[0].hunks[0].lines[0].text, "line1");
  EXPECT_EQ(result.files[0].hunks[0].lines[1].text, "line2");
  EXPECT_EQ(result.files[0].hunks[0].lines[2].text, "two");
}

TEST(DiffParser, MultiFileDiff) {
  const std::string diff =
      "diff --git a/a.txt b/a.txt\n"
      "--- a/a.txt\n"
      "+++ b/a.txt\n"
      "@@ -1 +1 @@\n"
      "-old\n"
      "+new\n"
      "diff --git a/b.txt b/b.txt\n"
      "--- a/b.txt\n"
      "+++ b/b.txt\n"
      "@@ -1 +1 @@\n"
      "-foo\n"
      "+bar\n";
  const auto result = UnifiedDiffParser::Parse(diff);
  EXPECT_TRUE(result.success);
  ASSERT_EQ(result.files.size(), 2u);
  EXPECT_EQ(result.files[0].old_path, "a.txt");
  EXPECT_EQ(result.files[1].old_path, "b.txt");
}

TEST(DiffParser, MultipleHunks) {
  const std::string diff =
      "diff --git a/file.txt b/file.txt\n"
      "--- a/file.txt\n"
      "+++ b/file.txt\n"
      "@@ -1,2 +1,3 @@\n"
      " line1\n"
      "+inserted\n"
      " line2\n"
      "@@ -9,2 +10,3 @@\n"
      " line9\n"
      "+another\n"
      " line10\n";
  const auto result = UnifiedDiffParser::Parse(diff);
  EXPECT_TRUE(result.success);
  ASSERT_EQ(result.files.size(), 1u);
  ASSERT_EQ(result.files[0].hunks.size(), 2u);
  EXPECT_EQ(result.files[0].hunks[0].old_start, 1);
  EXPECT_EQ(result.files[0].hunks[1].old_start, 9);
}

TEST(DiffParser, NoTrailingNewline) {
  const std::string diff =
      "diff --git a/file.txt b/file.txt\n"
      "--- a/file.txt\n"
      "+++ b/file.txt\n"
      "@@ -1,2 +1 @@\n"
      " line1\n"
      "-line2\n"
      "\\ No newline at end of file\n";
  const auto result = UnifiedDiffParser::Parse(diff);
  EXPECT_TRUE(result.success);
  ASSERT_EQ(result.files.size(), 1u);
  const DiffFile& file = result.files[0];
  EXPECT_TRUE(file.old_no_newline);
}

TEST(DiffParser, MissingLineCountsInHunkHeader) {
  // git omits ",1" when the count is 1.
  const std::string diff =
      "diff --git a/file.txt b/file.txt\n"
      "--- a/file.txt\n"
      "+++ b/file.txt\n"
      "@@ -1 +1 @@\n"
      "-x\n"
      "+y\n";
  const auto result = UnifiedDiffParser::Parse(diff);
  EXPECT_TRUE(result.success);
  ASSERT_EQ(result.files.size(), 1u);
  ASSERT_EQ(result.files[0].hunks.size(), 1u);
  EXPECT_EQ(result.files[0].hunks[0].old_count, 1);
  EXPECT_EQ(result.files[0].hunks[0].new_count, 1);
}

TEST(DiffParser, BodyLineStartingWithTripleDashOrPlus) {
  // In unified diff context lines are prefixed with a leading space, even when
  // the actual content starts with "--- " or "+++ ".
  const std::string diff =
      "diff --git a/test.txt b/test.txt\n"
      "--- a/test.txt\n"
      "+++ b/test.txt\n"
      "@@ -1,2 +1,2 @@\n"
      " --- old\n"
      " +++ new\n";
  const auto result = UnifiedDiffParser::Parse(diff);
  EXPECT_TRUE(result.success);
  ASSERT_EQ(result.files.size(), 1u);
  ASSERT_EQ(result.files[0].hunks.size(), 1u);
  ASSERT_EQ(result.files[0].hunks[0].lines.size(), 2u);
  EXPECT_EQ(result.files[0].hunks[0].lines[0].type, DiffLineType::kContext);
  EXPECT_EQ(result.files[0].hunks[0].lines[0].text, "--- old");
  EXPECT_EQ(result.files[0].hunks[0].lines[1].type, DiffLineType::kContext);
  EXPECT_EQ(result.files[0].hunks[0].lines[1].text, "+++ new");
}

TEST(DiffParser, QuotedPathWithSpaces) {
  const std::string diff =
      "diff --git \"a/path with spaces\" \"b/path with spaces\"\n"
      "--- \"a/path with spaces\"\n"
      "+++ \"b/path with spaces\"\n"
      "@@ -1 +1 @@\n"
      "-x\n"
      "+y\n";
  const auto result = UnifiedDiffParser::Parse(diff);
  EXPECT_TRUE(result.success);
  ASSERT_EQ(result.files.size(), 1u);
  EXPECT_EQ(result.files[0].old_path, "path with spaces");
}

TEST(DiffParser, TrailingBlankLinesIgnored) {
  const std::string diff =
      "diff --git a/a.txt b/a.txt\n"
      "--- a/a.txt\n"
      "+++ b/a.txt\n"
      "@@ -1 +1 @@\n"
      "-old\n"
      "+new\n"
      "\n"
      "\n";
  const auto result = UnifiedDiffParser::Parse(diff);
  EXPECT_TRUE(result.success);
  ASSERT_EQ(result.files.size(), 1u);
}

TEST(DiffParser, SingleLineContextAndSingleLineHunk) {
  const std::string diff =
      "diff --git a/file.txt b/file.txt\n"
      "--- a/file.txt\n"
      "+++ b/file.txt\n"
      "@@ -1,2 +1 @@\n"
      " line1\n"
      "-removed\n";
  const auto result = UnifiedDiffParser::Parse(diff);
  EXPECT_TRUE(result.success);
  ASSERT_EQ(result.files[0].hunks.size(), 1u);
  ASSERT_EQ(result.files[0].hunks[0].lines.size(), 2u);
  EXPECT_EQ(result.files[0].hunks[0].lines[0].type, DiffLineType::kContext);
  EXPECT_EQ(result.files[0].hunks[0].lines[1].type, DiffLineType::kRemoved);
}

TEST(DiffParser, OnlyAddedLines) {
  const std::string diff =
      "diff --git a/file.txt b/file.txt\n"
      "--- /dev/null\n"
      "+++ b/file.txt\n"
      "@@ -0,0 +1,3 @@\n"
      "+a\n"
      "+b\n"
      "+c\n";
  const auto result = UnifiedDiffParser::Parse(diff);
  EXPECT_TRUE(result.success);
  ASSERT_EQ(result.files[0].hunks.size(), 1u);
  ASSERT_EQ(result.files[0].hunks[0].lines.size(), 3u);
  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(result.files[0].hunks[0].lines[i].type, DiffLineType::kAdded);
    EXPECT_EQ(result.files[0].hunks[0].lines[i].new_line, i + 1);
    EXPECT_EQ(result.files[0].hunks[0].lines[i].old_line, std::nullopt);
  }
}

TEST(DiffParser, OnlyRemovedLines) {
  const std::string diff =
      "diff --git a/file.txt b/file.txt\n"
      "--- a/file.txt\n"
      "+++ /dev/null\n"
      "@@ -1,3 +0,0 @@\n"
      "-a\n"
      "-b\n"
      "-c\n";
  const auto result = UnifiedDiffParser::Parse(diff);
  EXPECT_TRUE(result.success);
  ASSERT_EQ(result.files[0].hunks.size(), 1u);
  ASSERT_EQ(result.files[0].hunks[0].lines.size(), 3u);
  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(result.files[0].hunks[0].lines[i].type, DiffLineType::kRemoved);
    EXPECT_EQ(result.files[0].hunks[0].lines[i].old_line, i + 1);
    EXPECT_EQ(result.files[0].hunks[0].lines[i].new_line, std::nullopt);
  }
}

TEST(DiffParser, MixedContentTypeFromFirstCharacter) {
  // Context, removed, added, then context again.
  const std::string diff =
      "diff --git a/file.txt b/file.txt\n"
      "--- a/file.txt\n"
      "+++ b/file.txt\n"
      "@@ -2,4 +2,4 @@\n"
      " context_before\n"
      "-deleted\n"
      "+inserted\n"
      " more_context\n";
  const auto result = UnifiedDiffParser::Parse(diff);
  EXPECT_TRUE(result.success);
  ASSERT_EQ(result.files[0].hunks.size(), 1u);
  ASSERT_EQ(result.files[0].hunks[0].lines.size(), 4u);
  EXPECT_EQ(result.files[0].hunks[0].lines[0].type, DiffLineType::kContext);
  EXPECT_EQ(result.files[0].hunks[0].lines[1].type, DiffLineType::kRemoved);
  EXPECT_EQ(result.files[0].hunks[0].lines[2].type, DiffLineType::kAdded);
  EXPECT_EQ(result.files[0].hunks[0].lines[3].type, DiffLineType::kContext);
}

TEST(DiffParser, HunkHeaderWithContextText) {
  const std::string diff =
      "diff --git a/file.txt b/file.txt\n"
      "--- a/file.txt\n"
      "+++ b/file.txt\n"
      "@@ -10,5 +11,7 @@ function signature() {\n"
      " a\n"
      "+b\n"
      " c\n";
  const auto result = UnifiedDiffParser::Parse(diff);
  EXPECT_TRUE(result.success);
  ASSERT_EQ(result.files[0].hunks.size(), 1u);
  EXPECT_EQ(result.files[0].hunks[0].context, "function signature() {");
}

TEST(DiffParser, MalformedHunkHeaderDoesNotCrash) {
  const std::string diff =
      "diff --git a/file.txt b/file.txt\n"
      "--- a/file.txt\n"
      "+++ b/file.txt\n"
      "@@ badly formed @@\n"
      "this line should be ignored\n";
  const auto result = UnifiedDiffParser::Parse(diff);
  EXPECT_TRUE(result.success);
  ASSERT_EQ(result.files.size(), 1u);
  ASSERT_EQ(result.files[0].hunks.size(), 1u);
  // Malformed header keeps raw text as context but does not consume body lines.
  EXPECT_EQ(result.files[0].hunks[0].lines.size(), 0u);
}

TEST(DiffParser, HunkWithLeadingEmptyLine) {
  const std::string diff =
      "diff --git a/file.txt b/file.txt\n"
      "--- a/file.txt\n"
      "+++ b/file.txt\n"
      "@@ -1,2 +1,2 @@\n"
      " \n"
      " line2\n";
  const auto result = UnifiedDiffParser::Parse(diff);
  EXPECT_TRUE(result.success);
  ASSERT_EQ(result.files[0].hunks.size(), 1u);
  ASSERT_EQ(result.files[0].hunks[0].lines.size(), 2u);
  EXPECT_EQ(result.files[0].hunks[0].lines[0].type, DiffLineType::kContext);
  EXPECT_EQ(result.files[0].hunks[0].lines[0].text, "");
}

TEST(DiffParser, RejectsOnlyAddedWithoutHunk) {
  const std::string diff =
      "+no diff header\n"
      "+another line\n";
  const auto result = UnifiedDiffParser::Parse(diff);
  EXPECT_TRUE(result.success);
  EXPECT_TRUE(result.files.empty());
}

TEST(DiffParser, ConsecutiveHunksHonourLineCounts) {
  // Second hunk starts after the first finishes.
  const std::string diff =
      "diff --git a/file.txt b/file.txt\n"
      "--- a/file.txt\n"
      "+++ b/file.txt\n"
      "@@ -1,3 +1,3 @@\n"
      " line1\n"
      "-line2\n"
      "+two\n"
      " line3\n"
      "@@ -5,3 +5,2 @@\n"
      " line5\n"
      "-line6\n"
      " line7\n";
  const auto result = UnifiedDiffParser::Parse(diff);
  EXPECT_TRUE(result.success);
  ASSERT_EQ(result.files[0].hunks.size(), 2u);
  ASSERT_EQ(result.files[0].hunks[0].lines.size(), 4u);
  ASSERT_EQ(result.files[0].hunks[1].lines.size(), 3u);
  EXPECT_EQ(result.files[0].hunks[1].lines[1].type, DiffLineType::kRemoved);
  EXPECT_EQ(result.files[0].hunks[1].lines[2].type, DiffLineType::kContext);
}

}  // namespace
}  // namespace terminal_ui_kit
