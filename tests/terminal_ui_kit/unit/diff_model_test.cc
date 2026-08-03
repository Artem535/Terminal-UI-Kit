#include "terminal_ui_kit/diff/diff_model.h"

#include <gtest/gtest.h>
#include <string_view>

namespace terminal_ui_kit {
namespace {

// ========================================================================
// Basic single-file, single-hunk tests
// ========================================================================

TEST(DiffModel, ParsesSimpleSingleFileDiff) {
  std::string_view input =
      "diff --git a/src/hello.cc b/src/hello.cc\n"
      "index abc1234..def5678 100644\n"
      "--- a/src/hello.cc\n"
      "+++ b/src/hello.cc\n"
      "@@ -1,3 +1,4 @@\n"
      " #include <iostream>\n"
      "-int main() {\n"
      "+int main(int argc, char* argv[]) {\n"
      "   std::cout << \"Hello\" << std::endl;\n"
      "}\n";
  auto model = DiffModel::parse(input);

  ASSERT_EQ(model.file_count(), 1u);
  EXPECT_EQ(model.files[0].old_path, "src/hello.cc");
  EXPECT_EQ(model.files[0].new_path, "src/hello.cc");
  EXPECT_EQ(model.files[0].type, DiffFileType::kNormal);
  ASSERT_EQ(model.files[0].hunks.size(), 1u);

  auto& hunk = model.files[0].hunks[0];
  EXPECT_EQ(hunk.old_start, 1);
  EXPECT_EQ(hunk.old_count, 3);
  EXPECT_EQ(hunk.new_start, 1);
  EXPECT_EQ(hunk.new_count, 4);
  // 1 hunk header + 4 data lines = 5 total
  EXPECT_EQ(hunk.total_lines(), 5u);

  EXPECT_EQ(model.addition_count(), 1u);
  EXPECT_EQ(model.deletion_count(), 1u);
  EXPECT_EQ(model.context_count(), 2u);
  EXPECT_EQ(model.line_count(), 5u);
}

TEST(DiffModel, ParsesOldCountEqualsOneImplicitly) {
  std::string_view input =
      "diff --git a/x.cc b/x.cc\n"
      "@@ -1 +1,2 @@\n"
      "-x\n"
      "+x\n"
      "+y\n";
  auto model = DiffModel::parse(input);
  ASSERT_EQ(model.file_count(), 1u);
  ASSERT_EQ(model.files[0].hunks.size(), 1u);
  auto& hunk = model.files[0].hunks[0];
  EXPECT_EQ(hunk.old_start, 1);
  EXPECT_EQ(hunk.old_count, 1);
  EXPECT_EQ(hunk.new_start, 1);
  EXPECT_EQ(hunk.new_count, 2);
}

// ========================================================================
// Line number tracking
// ========================================================================

TEST(DiffModel, TracksOldAndNewLineNumbers) {
  std::string_view input =
      "diff --git a/f.cc b/f.cc\n"
      "@@ -5,2 +5,3 @@\n"
      " line5\n"       // context: old=5, new=5
      "-line6\n"      // deletion: old=6
      "+line6-new\n"  // addition: new=6
      "+line6-new2\n" // addition: new=7
      " line7\n";     // context: old=7, new=8

  auto model = DiffModel::parse(input);
  ASSERT_EQ(model.file_count(), 1u);
  ASSERT_EQ(model.files[0].hunks.size(), 1u);

  auto& lines = model.files[0].hunks[0].lines;
  // Skip hunk header (index 0)
  // Line 1: context " line5"
  EXPECT_EQ(lines[1].change_type, DiffChangeType::kContext);
  EXPECT_EQ(lines[1].old_line_no, 5);
  EXPECT_EQ(lines[1].new_line_no, 5);
  // Line 2: deletion "-line6"
  EXPECT_EQ(lines[2].change_type, DiffChangeType::kDeletion);
  EXPECT_EQ(lines[2].old_line_no, 6);
  EXPECT_FALSE(lines[2].new_line_no.has_value());
  // Line 3: addition "+line6-new"
  EXPECT_EQ(lines[3].change_type, DiffChangeType::kAddition);
  EXPECT_FALSE(lines[3].old_line_no.has_value());
  EXPECT_EQ(lines[3].new_line_no, 6);
  // Line 4: addition "+line6-new2"
  EXPECT_EQ(lines[4].change_type, DiffChangeType::kAddition);
  EXPECT_FALSE(lines[4].old_line_no.has_value());
  EXPECT_EQ(lines[4].new_line_no, 7);
  // Line 5: context " line7"
  EXPECT_EQ(lines[5].change_type, DiffChangeType::kContext);
  EXPECT_EQ(lines[5].old_line_no, 7);
  EXPECT_EQ(lines[5].new_line_no, 8);
}

// ========================================================================
// Multiple files
// ========================================================================

TEST(DiffModel, ParsesMultipleFiles) {
  std::string_view input =
      "diff --git a/file1.txt b/file1.txt\n"
      "index 111..222 100644\n"
      "--- a/file1.txt\n"
      "+++ b/file1.txt\n"
      "@@ -1 +1,2 @@\n"
      "-old\n"
      "+new1\n"
      "+new2\n"
      "\n"
      "diff --git a/file2.txt b/file2.txt\n"
      "--- a/file2.txt\n"
      "+++ b/file2.txt\n"
      "@@ -1 +1 @@\n"
      "-deleted\n"
      "+kept\n";

  auto model = DiffModel::parse(input);
  ASSERT_EQ(model.file_count(), 2u);

  EXPECT_EQ(model.files[0].old_path, "file1.txt");
  EXPECT_EQ(model.files[1].old_path, "file2.txt");

  EXPECT_EQ(model.files[0].hunks.size(), 1u);
  EXPECT_EQ(model.files[1].hunks.size(), 1u);

  // file1: 1 deletion + 2 additions = 3 data lines + 1 header = 5 lines
  // file2: 1 deletion + 1 addition = 2 data lines + 1 header = 3 lines
  EXPECT_EQ(model.addition_count(), 3u);
  EXPECT_EQ(model.deletion_count(), 2u);
}

// ========================================================================
// Binary files
// ========================================================================

TEST(DiffModel, ParsesBinaryFiles) {
  std::string_view input =
      "diff --git a/image.png b/image.png\n"
      "index aabbcc..112233 100644\n"
      "Binary files a/image.png and b/image.png differ\n";

  auto model = DiffModel::parse(input);
  ASSERT_EQ(model.file_count(), 1u);
  EXPECT_EQ(model.files[0].type, DiffFileType::kBinary);
  EXPECT_EQ(model.files[0].old_path, "image.png");
  EXPECT_EQ(model.files[0].new_path, "image.png");
  EXPECT_EQ(model.files[0].hunks.size(), 0u);
}

// ========================================================================
// New file and deleted file
// ========================================================================

TEST(DiffModel, ParsesNewFile) {
  std::string_view input =
      "diff --git a/new.cc b/new.cc\n"
      "new file mode 100644\n"
      "index 0000000..aabbccd\n"
      "--- /dev/null\n"
      "+++ b/new.cc\n"
      "@@ -0,0 +1,3 @@\n"
      "+#include <stdio.h>\n"
      "+\n"
      "+int main() {}\n";

  auto model = DiffModel::parse(input);
  ASSERT_EQ(model.file_count(), 1u);
  EXPECT_EQ(model.files[0].type, DiffFileType::kNewFile);
  // Path comes from diff --git header
  EXPECT_EQ(model.files[0].old_path, "new.cc");
  EXPECT_EQ(model.files[0].new_path, "new.cc");
  ASSERT_EQ(model.files[0].hunks.size(), 1u);
  EXPECT_EQ(model.files[0].hunks[0].old_start, 0);
  EXPECT_EQ(model.files[0].hunks[0].new_start, 1);
  EXPECT_EQ(model.addition_count(), 3u);
}

TEST(DiffModel, ParsesDeletedFile) {
  std::string_view input =
      "diff --git a/old.cc b/old.cc\n"
      "deleted file mode 100644\n"
      "index aabbccd..0000000\n"
      "--- a/old.cc\n"
      "+++ /dev/null\n"
      "@@ -1,2 +0,0 @@\n"
      "-line1\n"
      "-line2\n";

  auto model = DiffModel::parse(input);
  ASSERT_EQ(model.file_count(), 1u);
  EXPECT_EQ(model.files[0].type, DiffFileType::kDeletedFile);
  EXPECT_EQ(model.files[0].old_path, "old.cc");
  EXPECT_EQ(model.files[0].new_path, "old.cc");
}

// ========================================================================
// Rename and copy
// ========================================================================

TEST(DiffModel, ParsesRename) {
  std::string_view input =
      "diff --git a/old.txt b/new.txt\n"
      "similarity index 100%\n"
      "rename from old.txt\n"
      "rename to new.txt\n";

  auto model = DiffModel::parse(input);
  ASSERT_EQ(model.file_count(), 1u);
  EXPECT_EQ(model.files[0].type, DiffFileType::kRenamedFile);
  EXPECT_EQ(model.files[0].old_path, "old.txt");
  EXPECT_EQ(model.files[0].new_path, "new.txt");
}

TEST(DiffModel, ParsesCopy) {
  std::string_view input =
      "diff --git a/src/c.cc b/backup/c.cc\n"
      "similarity index 100%\n"
      "copy from src/c.cc\n"
      "copy to backup/c.cc\n";

  auto model = DiffModel::parse(input);
  ASSERT_EQ(model.file_count(), 1u);
  EXPECT_EQ(model.files[0].type, DiffFileType::kCopyFile);
  EXPECT_EQ(model.files[0].old_path, "src/c.cc");
  EXPECT_EQ(model.files[0].new_path, "backup/c.cc");
}

// ========================================================================
// Hunk header with text after @@
// ========================================================================

TEST(DiffModel, ParsesHunkHeaderWithDescription) {
  std::string_view input =
      "diff --git a/f.cc b/f.cc\n"
      "@@ -1 +1,2 @@ (some description)\n"
      "-x\n"
      "+x\n"
      "+y\n";

  auto model = DiffModel::parse(input);
  ASSERT_EQ(model.file_count(), 1u);
  ASSERT_EQ(model.files[0].hunks.size(), 1u);
  EXPECT_EQ(model.files[0].hunks[0].old_start, 1);
  EXPECT_EQ(model.files[0].hunks[0].new_start, 1);
}

// ========================================================================
// Malformed hunk headers are skipped gracefully
// ========================================================================

TEST(DiffModel, SkipsMalformedHunkHeader) {
  std::string_view input =
      "diff --git a/f.cc b/f.cc\n"
      "@@ bad header @@\n"
      " some text\n";

  auto model = DiffModel::parse(input);
  ASSERT_EQ(model.file_count(), 1u);
  // The hunk header is malformed so no hunk should be created.
  // The " some text" line starts with ' ' but there's no current hunk,
  // so it is skipped.
  EXPECT_EQ(model.files[0].hunks.size(), 0u);
}

// ========================================================================
// Multiple hunks per file
// ========================================================================

TEST(DiffModel, ParsesMultipleHunksPerFile) {
  std::string_view input =
      "diff --git a/f.cc b/f.cc\n"
      "@@ -1,1 +1,2 @@\n"
      "-a\n"
      "+b\n"
      "+c\n"
      "@@ -10,1 +12,0 @@\n"
      "-line10\n";

  auto model = DiffModel::parse(input);
  ASSERT_EQ(model.file_count(), 1u);
  ASSERT_EQ(model.files[0].hunks.size(), 2u);
  EXPECT_EQ(model.files[0].hunks[0].old_start, 1);
  EXPECT_EQ(model.files[0].hunks[1].old_start, 10);
}

// ========================================================================
// Hunk header line is preserved
// ========================================================================

TEST(DiffModel, HunkHeaderLinePreserved) {
  std::string_view input =
      "diff --git a/f.cc b/f.cc\n"
      "@@ -5 +5 @@\n"
      "-x\n";

  auto model = DiffModel::parse(input);
  ASSERT_EQ(model.file_count(), 1u);
  ASSERT_EQ(model.files[0].hunks.size(), 1u);
  auto& hunk = model.files[0].hunks[0];

  // First line of the hunk should be the header
  EXPECT_EQ(hunk.lines[0].change_type, DiffChangeType::kHunkHeader);
  EXPECT_EQ(hunk.lines[0].text, "@@ -5 +5 @@");
}

// ========================================================================
// File path handling
// ========================================================================

TEST(DiffModel, ParsesGitFilePathsCorrectly) {
  std::string_view input =
      "diff --git a/src/lib/utils/helper.cc b/src/lib/utils/helper.cc\n"
      "--- a/src/lib/utils/helper.cc\n"
      "+++ b/src/lib/utils/helper.cc\n"
      "@@ -1 +1 @@\n"
      "-old\n"
      "+new\n";

  auto model = DiffModel::parse(input);
  ASSERT_EQ(model.file_count(), 1u);
  EXPECT_EQ(model.files[0].old_path, "src/lib/utils/helper.cc");
  EXPECT_EQ(model.files[0].new_path, "src/lib/utils/helper.cc");
}

// ========================================================================
// Count helpers
// ========================================================================

TEST(DiffModel, CountHelpersReturnZeroForEmpty) {
  auto model = DiffModel::parse("");
  EXPECT_EQ(model.file_count(), 0u);
  EXPECT_EQ(model.hunk_count(), 0u);
  EXPECT_EQ(model.line_count(), 0u);
  EXPECT_EQ(model.addition_count(), 0u);
  EXPECT_EQ(model.deletion_count(), 0u);
  EXPECT_EQ(model.context_count(), 0u);
}

TEST(DiffModel, CountHelpersAreCorrect) {
  std::string_view input =
      "diff --git a/f.cc b/f.cc\n"
      "@@ -1,3 +1,5 @@\n"
      " c1\n"
      "-d1\n"
      "-d2\n"
      "+a1\n"
      "+a2\n"
      " c2\n";

  auto model = DiffModel::parse(input);
  EXPECT_EQ(model.addition_count(), 2u);
  EXPECT_EQ(model.deletion_count(), 2u);
  EXPECT_EQ(model.context_count(), 2u);
  // 2 header + 5 data = 7 lines
  EXPECT_EQ(model.line_count(), 7u);
}

// ========================================================================
// Edge case: hunk with implicit counts
// ========================================================================

TEST(DiffModel, ParsesHunkWithImplicitCounts) {
  std::string_view input =
      "diff --git a/f.cc b/f.cc\n"
      "@@ -10 +10,2 @@\n"
      "-old_line\n"
      "+new_line1\n"
      "+new_line2\n";

  auto model = DiffModel::parse(input);
  ASSERT_EQ(model.file_count(), 1u);
  ASSERT_EQ(model.files[0].hunks.size(), 1u);
  EXPECT_EQ(model.files[0].hunks[0].old_start, 10);
  EXPECT_EQ(model.files[0].hunks[0].old_count, 1);
  EXPECT_EQ(model.files[0].hunks[0].new_start, 10);
  EXPECT_EQ(model.files[0].hunks[0].new_count, 2);
}

// ========================================================================
// Edge case: new file with @@ -0,0 +1,N @@
// ========================================================================

TEST(DiffModel, ParsesNewFileHunk) {
  std::string_view input =
      "diff --git a/empty.cc b/empty.cc\n"
      "new file mode 100644\n"
      "--- /dev/null\n"
      "+++ b/empty.cc\n"
      "@@ -0,0 +1,1 @@\n"
      "+content\n";

  auto model = DiffModel::parse(input);
  ASSERT_EQ(model.file_count(), 1u);
  ASSERT_EQ(model.files[0].hunks.size(), 1u);
  EXPECT_EQ(model.files[0].hunks[0].old_start, 0);
  EXPECT_EQ(model.files[0].hunks[0].old_count, 0);
  EXPECT_EQ(model.files[0].hunks[0].new_start, 1);
  EXPECT_EQ(model.files[0].hunks[0].new_count, 1);
  // hunk header + 1 addition = 2 lines
  ASSERT_EQ(model.files[0].hunks[0].lines.size(), 2u);
  EXPECT_EQ(model.files[0].hunks[0].lines[1].text, "content");
}

// ========================================================================
// Edge case: context diff format (***)
// ========================================================================

TEST(DiffModel, ParsesContextDiffFormat) {
  std::string_view input =
      "*** file.txt    2024-01-01 12:00:00\n"
      "--- file.txt    2024-01-02 12:00:00\n";

  auto model = DiffModel::parse(input);
  ASSERT_EQ(model.file_count(), 1u);
  // The *** line extracts the filename.
  // The --- line has a date, not a path — it's a context diff separator.
  EXPECT_EQ(model.files[0].old_path, "file.txt");
}

// ========================================================================
// Line text preservation
// ========================================================================

TEST(DiffModel, PreservesLineTextAccurately) {
  std::string_view input =
      "diff --git a/f.cc b/f.cc\n"
      "@@ -1 +1,3 @@\n"
      "-  tabs and   spaces\n"
      "+  new line 1\n"
      "+  new line 2\n"
      " +context with special chars !@#$%\n";

  auto model = DiffModel::parse(input);
  ASSERT_EQ(model.file_count(), 1u);
  auto& lines = model.files[0].hunks[0].lines;

  // Skip hunk header (index 0)
  EXPECT_EQ(lines[1].text, "  tabs and   spaces");
  EXPECT_EQ(lines[2].text, "  new line 1");
  EXPECT_EQ(lines[3].text, "  new line 2");
  // Context lines start with ' ', so text = everything after ' '
  // The input line is " +context..." — after removing the ' ' prefix:
  // "+context with special chars !@#$%"
  EXPECT_EQ(lines[4].text, "+context with special chars !@#$%");
}

// ========================================================================
// Edge case: empty hunk (no lines between headers)
// ========================================================================

TEST(DiffModel, HandlesFileWithOnlyHeaderNoLines) {
  std::string_view input =
      "diff --git a/f.cc b/f.cc\n"
      "index abc..def 100644\n"
      "--- a/f.cc\n"
      "+++ b/f.cc\n";

  auto model = DiffModel::parse(input);
  ASSERT_EQ(model.file_count(), 1u);
  EXPECT_EQ(model.files[0].hunks.size(), 0u);
  EXPECT_EQ(model.line_count(), 0u);
}

// ========================================================================
// Edge case: trailing whitespace in lines preserved
// ========================================================================

TEST(DiffModel, PreservesTrailingWhitespaceInDiffLines) {
  std::string_view input =
      "diff --git a/f.cc b/f.cc\n"
      "@@ -1 +1 @@\n"
      "-line with trailing   \n"
      "+new line with trailing   \n";

  auto model = DiffModel::parse(input);
  ASSERT_EQ(model.file_count(), 1u);
  auto& hunk = model.files[0].hunks[0];
  ASSERT_GE(hunk.lines.size(), 3u);  // header + 2 data lines

  // The parser strips trailing \r and \n from line boundaries, but not
  // from the diff content itself after removing the prefix character.
  // Trailing spaces in the original diff content are preserved.
  EXPECT_EQ(hunk.lines[1].text, "line with trailing   ");
  EXPECT_EQ(hunk.lines[2].text, "new line with trailing   ");
}

// ========================================================================
// Edge case: hunk with zero count (empty old/new)
// ========================================================================

TEST(DiffModel, ParsesHunkWithZeroCounts) {
  std::string_view input =
      "diff --git a/f.cc b/f.cc\n"
      "@@ -0,0 +1,2 @@\n"
      "+line1\n"
      "+line2\n";

  auto model = DiffModel::parse(input);
  ASSERT_EQ(model.file_count(), 1u);
  ASSERT_EQ(model.files[0].hunks.size(), 1u);
  EXPECT_EQ(model.files[0].hunks[0].old_start, 0);
  EXPECT_EQ(model.files[0].hunks[0].old_count, 0);
  EXPECT_EQ(model.files[0].hunks[0].new_start, 1);
  EXPECT_EQ(model.files[0].hunks[0].new_count, 2);
}

// ========================================================================
// Edge case: no diff --git prefix (pure unified diff)
// ========================================================================

TEST(DiffModel, ParsesPureUnifiedDiffWithoutGitHeader) {
  std::string_view input =
      "--- a/file.txt\t2024-01-01 12:00:00\n"
      "+++ b/file.txt\t2024-01-02 12:00:00\n"
      "@@ -1 +1 @@\n"
      "-old\n"
      "+new\n";

  auto model = DiffModel::parse(input);
  ASSERT_EQ(model.file_count(), 1u);
  EXPECT_EQ(model.files[0].old_path, "file.txt");
  EXPECT_EQ(model.files[0].new_path, "file.txt");
  ASSERT_EQ(model.files[0].hunks.size(), 1u);
  EXPECT_EQ(model.files[0].hunks[0].lines.size(), 3u);
}

// ========================================================================
// Edge case: multiple consecutive deletions/additions
// ========================================================================

TEST(DiffModel, HandlesConsecutiveSameTypeLines) {
  std::string_view input =
      "diff --git a/f.cc b/f.cc\n"
      "@@ -1,3 +1,5 @@\n"
      " context\n"
      "-del1\n"
      "-del2\n"
      "-del3\n"
      "+add1\n"
      "+add2\n";

  auto model = DiffModel::parse(input);
  ASSERT_EQ(model.file_count(), 1u);
  auto& hunk = model.files[0].hunks[0];
  EXPECT_EQ(model.addition_count(), 2u);
  EXPECT_EQ(model.deletion_count(), 3u);
  EXPECT_EQ(model.context_count(), 1u);
}

}  // namespace
}  // namespace terminal_ui_kit
