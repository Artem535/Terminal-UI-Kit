#include "terminal_ui_kit/diff/diff_parser.h"

#include <string>
#include <vector>

#include "terminal_ui_kit/diff/diff_model.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace diff {
namespace {

class DiffParserTest : public testing::Test {};

// --- Basic parsing --------------------------------------------------------

TEST_F(DiffParserTest, ParsesSimpleDiff) {
  const char* input =
      "diff --git a/foo.txt b/foo.txt\n"
      "index abc123..def456 100644\n"
      "--- a/foo.txt\n"
      "+++ b/foo.txt\n"
      "@@ -1,2 +1,3 @@\n"
      " line1\n"
      "+new line\n"
      " line2\n";

  const auto diff = parse_unified_diff(input);

  ASSERT_EQ(diff.files.size(), 1u);
  EXPECT_EQ(diff.files[0].old_path, "foo.txt");
  EXPECT_EQ(diff.files[0].new_path, "foo.txt");
  EXPECT_TRUE(diff.files[0].hunks.empty() == false);

  auto& hunk = diff.files[0].hunks[0];
  EXPECT_EQ(hunk.old_start, 1u);
  EXPECT_EQ(hunk.old_length, 2u);
  EXPECT_EQ(hunk.new_start, 1u);
  EXPECT_EQ(hunk.new_length, 3u);

  EXPECT_EQ(hunk.lines.size(), 3u);
  EXPECT_EQ(hunk.lines[0].type, LineType::kContext);
  EXPECT_EQ(hunk.lines[0].content, "line1");
  EXPECT_EQ(hunk.lines[1].type, LineType::kAddition);
  EXPECT_EQ(hunk.lines[1].content, "new line");
  EXPECT_EQ(hunk.lines[2].type, LineType::kContext);
  EXPECT_EQ(hunk.lines[2].content, "line2");
}

// --- Multiple hunks in one file -------------------------------------------

TEST_F(DiffParserTest, ParsesMultipleHunks) {
  const char* input =
      "diff --git a/multi.txt b/multi.txt\n"
      "--- a/multi.txt\n"
      "+++ b/multi.txt\n"
      "@@ -1,2 +1,3 @@\n"
      " A\n"
      "+B\n"
      " C\n"
      "@@ -10,2 +11,0 @@\n"
      "- old_deleted\n";

  const auto diff = parse_unified_diff(input);

  ASSERT_EQ(diff.files.size(), 1u);
  EXPECT_EQ(diff.files[0].hunks.size(), 2u);

  EXPECT_EQ(diff.files[0].hunks[0].old_start, 1u);
  EXPECT_EQ(diff.files[0].hunks[0].new_start, 1u);
  EXPECT_EQ(diff.files[0].hunks[1].old_start, 10u);
  EXPECT_EQ(diff.files[0].hunks[1].new_start, 11u);
}

// --- Multiple files -------------------------------------------------------

TEST_F(DiffParserTest, ParsesMultipleFiles) {
  const char* input =
      "diff --git a/file1.txt b/file1.txt\n"
      "--- a/file1.txt\n"
      "+++ b/file1.txt\n"
      "@@ -1 +1,2 @@\n"
      "- old1\n"
      "+new1\n"
      "+new1b\n"
      "diff --git a/file2.txt b/file2.txt\n"
      "--- a/file2.txt\n"
      "+++ b/file2.txt\n"
      "@@ -1 +1 @@\n"
      " same\n";

  const auto diff = parse_unified_diff(input);

  ASSERT_EQ(diff.files.size(), 2u);
  EXPECT_EQ(diff.files[0].old_path, "file1.txt");
  EXPECT_EQ(diff.files[0].new_path, "file1.txt");
  EXPECT_EQ(diff.files[0].hunks[0].lines.size(), 3u);

  EXPECT_EQ(diff.files[1].old_path, "file2.txt");
  EXPECT_EQ(diff.files[1].new_path, "file2.txt");
  EXPECT_EQ(diff.files[1].hunks[0].lines.size(), 1u);
}

// --- Empty diff -----------------------------------------------------------

TEST_F(DiffParserTest, ReturnsEmptyForEmptyInput) {
  const auto diff = parse_unified_diff("");
  EXPECT_TRUE(diff.files.empty());
}

// --- Binary file ----------------------------------------------------------

TEST_F(DiffParserTest, RecognizesBinaryFile) {
  const char* input =
      "diff --git a/image.png b/image.png\n"
      "Binary files a/image.png and b/image.png differ\n";

  const auto diff = parse_unified_diff(input);

  ASSERT_EQ(diff.files.size(), 1u);
  EXPECT_TRUE(diff.files[0].is_binary);
  EXPECT_EQ(diff.files[0].old_path, "image.png");
  EXPECT_EQ(diff.files[0].new_path, "image.png");
  EXPECT_TRUE(diff.files[0].hunks.empty());
}

// --- Created file (old = dev/null) ----------------------------------------

TEST_F(DiffParserTest, CreatesFileFromDevNull) {
  const char* input =
      "diff --git a/new.py b/new.py\n"
      "new file mode 100644\n"
      "--- /dev/null\n"
      "+++ b/new.py\n"
      "@@ -0,0 +1,3 @@\n"
      "+def hello():\n"
      "+    pass\n"
      "+\n";

  const auto diff = parse_unified_diff(input);

  ASSERT_EQ(diff.files.size(), 1u);
  EXPECT_EQ(diff.files[0].old_path, "/dev/null");
  EXPECT_EQ(diff.files[0].new_path, "new.py");
  EXPECT_FALSE(diff.files[0].is_binary);
  EXPECT_EQ(diff.files[0].hunks.size(), 1u);
  EXPECT_EQ(diff.files[0].hunks[0].lines.size(), 3u);
}

// --- Deleted file (new = dev/null) ----------------------------------------

TEST_F(DiffParserTest, DeletesFileToDevNull) {
  const char* input =
      "diff --git a/removed.txt b/removed.txt\n"
      "deleted file mode 100644\n"
      "--- a/removed.txt\n"
      "+++ /dev/null\n"
      "@@ -1 +0,0 @@\n"
      "- bye\n";

  const auto diff = parse_unified_diff(input);

  ASSERT_EQ(diff.files.size(), 1u);
  EXPECT_EQ(diff.files[0].new_path, "/dev/null");
  EXPECT_EQ(diff.files[0].hunks[0].lines.size(), 1u);
  EXPECT_EQ(diff.files[0].hunks[0].lines[0].type, LineType::kDeletion);
}

// --- Malformed hunks are ignored gracefully --------------------------------

TEST_F(DiffParserTest, HandlesMalformedHunk) {
  const char* input =
      "diff --git a/bad.txt b/bad.txt\n"
      "--- a/bad.txt\n"
      "+++ b/bad.txt\n"
      "this is not a hunk header\n"
      "context should be ignored\n"
      "@@ -1,2 +1,2 @@\n"
      " ok\n"
      "\n";

  const auto diff = parse_unified_diff(input);

  // The malformed header line is not parsed, so no hunk is created until the
  // real one appears.  Only the good hunk survives.
  ASSERT_EQ(diff.files.size(), 1u);
  EXPECT_EQ(diff.files[0].hunks.size(), 1u);
  EXPECT_EQ(diff.files[0].hunks[0].lines.size(), 1u);
}

// --- Only context lines ---------------------------------------------------

TEST_F(DiffParserTest, HandlesHunkWithOnlyContext) {
  const char* input =
      "diff --git a/noop.txt b/noop.txt\n"
      "--- a/noop.txt\n"
      "+++ b/noop.txt\n"
      "@@ -1 +1 @@\n"
      " unchanged\n";

  const auto diff = parse_unified_diff(input);

  ASSERT_EQ(diff.files.size(), 1u);
  EXPECT_EQ(diff.files[0].hunks[0].lines.size(), 1u);
  EXPECT_EQ(diff.files[0].hunks[0].lines[0].type, LineType::kContext);
}

// --- Line preservation: content without leading markers -------------------

TEST_F(DiffParserTest, StripsLeadingMarkers) {
  const char* input =
      "@@ -1,3 +1,3 @@\n"
      "-minus\n"
      "+plus\n"
      " ctx\n";

  const auto diff = parse_unified_diff(input);

  ASSERT_EQ(diff.files.size(), 1u);
  auto& ln = diff.files[0].hunks[0].lines;
  EXPECT_EQ(ln[0].type, LineType::kDeletion);
  EXPECT_EQ(ln[0].content, "minus");
  EXPECT_EQ(ln[1].type, LineType::kAddition);
  EXPECT_EQ(ln[1].content, "plus");
  EXPECT_EQ(ln[2].type, LineType::kContext);
  EXPECT_EQ(ln[2].content, "ctx");
}

// --- Empty input files (length = 0) ---------------------------------------

TEST_F(DiffParserTest, ParsesZeroLengthHunks) {
  const char* input =
      "@@ -0,0 +1,1 @@\n"
      "+new line\n";

  const auto diff = parse_unified_diff(input);

  ASSERT_EQ(diff.files.size(), 1u);
  EXPECT_EQ(diff.files[0].hunks[0].old_start, 0u);
  EXPECT_EQ(diff.files[0].hunks[0].old_length, 0u);
  EXPECT_EQ(diff.files[0].hunks[0].new_length, 1u);
}

// --- CR-LF line endings ---------------------------------------------------

TEST_F(DiffParserTest, HandlesCRLF) {
  const char* input =
      "diff --git a/crlf.txt b/crlf.txt\r\n"
      "--- a/crlf.txt\r\n"
      "+++ b/crlf.txt\r\n"
      "@@ -1 +1,2 @@\r\n"
      " old\r\n"
      "+new\r\n";

  const auto diff = parse_unified_diff(input);

  ASSERT_EQ(diff.files.size(), 1u);
  EXPECT_EQ(diff.files[0].hunks[0].lines.size(), 2u);
  EXPECT_EQ(diff.files[0].hunks[0].lines[0].content, "old");
  EXPECT_EQ(diff.files[0].hunks[0].lines[1].content, "new");
}

// --- Hunk header is preserved ---------------------------------------------

TEST_F(DiffParserTest, PreservesHunkHeader) {
  const char* input =
      "@@ -10,3 +10,4 @@\n"
      " a\n";

  const auto diff = parse_unified_diff(input);

  ASSERT_EQ(diff.files.size(), 1u);
  EXPECT_EQ(diff.files[0].hunks[0].header, "@@ -10,3 +10,4 @@");
}

// --- No hunk-header lines at all (pure header) ----------------------------

TEST_F(DiffParserTest, HandlesFileWithOnlyHeader) {
  const char* input =
      "diff --git a/only_head.txt b/only_head.txt\n"
      "index abc..def 100644\n";

  const auto diff = parse_unified_diff(input);

  ASSERT_EQ(diff.files.size(), 1u);
  EXPECT_TRUE(diff.files[0].hunks.empty());
  EXPECT_EQ(diff.files[0].old_path, "only_head.txt");
}

// --- Extra spaces in hunk index -------------------------------------------

TEST_F(DiffParserTest, ToleratesSpaceInHunkFooter) {
  const char* input =
      "@@ -1,1 +1,1 @@ foo\n"
      " line\n";

  const auto diff = parse_unified_diff(input);

  ASSERT_EQ(diff.files.size(), 1u);
  EXPECT_EQ(diff.files[0].hunks[0].lines.size(), 1u);
}

}  // namespace
}  // namespace diff
}  // namespace terminal_ui_kit
