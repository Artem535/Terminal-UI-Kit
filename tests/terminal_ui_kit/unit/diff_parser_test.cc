#include "terminal_ui_kit/diff/diff_parser.h"

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

// --- Empty / edge cases -----------------------------------------------------

TEST(UnifiedDiffParser, EmptyInputReturnsEmptyDocument) {
  EXPECT_TRUE(parse_unified_diff("").files.empty());
  EXPECT_TRUE(parse_unified_diff("   ").files.empty());
}

TEST(UnifiedDiffParser, GarbageInputReturnsEmptyDocument) {
  // Random text without any diff structure.
  EXPECT_TRUE(parse_unified_diff("this is not a diff\n").files.empty());
}

// --- Basic single-file diff -------------------------------------------------

TEST(UnifiedDiffParser, SingleFileAddition) {
  const std::string input =
      "--- a/hello.txt\n"
      "+++ b/hello.txt\n"
      "@@ -0,0 +1 @@\n"
      "+Hello World\n";

  const auto doc = parse_unified_diff(input);

  ASSERT_EQ(doc.files.size(), 1u);
  EXPECT_EQ(doc.files[0].old_path, "hello.txt");
  EXPECT_EQ(doc.files[0].new_path, "hello.txt");
  ASSERT_EQ(doc.files[0].hunks.size(), 1u);
  EXPECT_EQ(doc.files[0].hunks[0].lines.size(), 1u);
  EXPECT_EQ(doc.files[0].hunks[0].lines[0].kind, DiffLineKind::kAddition);
  EXPECT_EQ(doc.files[0].hunks[0].lines[0].text, "Hello World");
}

TEST(UnifiedDiffParser, SingleFileDeletion) {
  const std::string input =
      "--- a/hello.txt\n"
      "+++ b/hello.txt\n"
      "@@ -1 +0,0 @@\n"
      "-Hello World\n";

  const auto doc = parse_unified_diff(input);

  ASSERT_EQ(doc.files.size(), 1u);
  ASSERT_EQ(doc.files[0].hunks.size(), 1u);
  EXPECT_EQ(doc.files[0].hunks[0].lines.size(), 1u);
  EXPECT_EQ(doc.files[0].hunks[0].lines[0].kind, DiffLineKind::kDeletion);
  EXPECT_EQ(doc.files[0].hunks[0].lines[0].text, "Hello World");
}

TEST(UnifiedDiffParser, SingleFileWithContextAndChanges) {
  const std::string input =
      "--- a/file.txt\n"
      "+++ b/file.txt\n"
      "@@ -1,5 +1,6 @@\n"
      " line one\n"
      "-line two\n"
      "+line two modified\n"
      " line three\n"
      "+new line\n"
      " line four\n";

  const auto doc = parse_unified_diff(input);

  ASSERT_EQ(doc.files.size(), 1u);
  ASSERT_EQ(doc.files[0].hunks.size(), 1u);
  auto& hunk = doc.files[0].hunks[0];

  EXPECT_EQ(hunk.old_start, 1);
  EXPECT_EQ(hunk.old_count, 5);
  EXPECT_EQ(hunk.new_start, 1);
  EXPECT_EQ(hunk.new_count, 6);
  EXPECT_EQ(hunk.lines.size(), 6u);

  EXPECT_EQ(hunk.lines[0].kind, DiffLineKind::kContext);
  EXPECT_EQ(hunk.lines[0].text, "line one");

  EXPECT_EQ(hunk.lines[1].kind, DiffLineKind::kDeletion);
  EXPECT_EQ(hunk.lines[1].text, "line two");

  EXPECT_EQ(hunk.lines[2].kind, DiffLineKind::kAddition);
  EXPECT_EQ(hunk.lines[2].text, "line two modified");

  EXPECT_EQ(hunk.lines[3].kind, DiffLineKind::kContext);
  EXPECT_EQ(hunk.lines[4].kind, DiffLineKind::kAddition);
  EXPECT_EQ(hunk.lines[5].kind, DiffLineKind::kContext);
}

// --- Multi-file diff --------------------------------------------------------

TEST(UnifiedDiffParser, MultipleFiles) {
  const std::string input =
      "diff --git a/file1.txt b/file1.txt\n"
      "index abc123..def456 100644\n"
      "--- a/file1.txt\n"
      "+++ b/file1.txt\n"
      "@@ -1 +1 @@\n"
      "-old\n"
      "+new\n"
      "\n"
      "diff --git a/file2.txt b/file2.txt\n"
      "index 789abc..123def 100644\n"
      "--- a/file2.txt\n"
      "+++ b/file2.txt\n"
      "@@ -1,2 +1,2 @@\n"
      " ctx1\n"
      "-old2\n"
      "+new2\n";

  const auto doc = parse_unified_diff(input);

  ASSERT_EQ(doc.files.size(), 2u);
  EXPECT_EQ(doc.files[0].old_path, "file1.txt");
  EXPECT_EQ(doc.files[1].old_path, "file2.txt");

  EXPECT_EQ(doc.files[0].hunks.size(), 1u);
  EXPECT_EQ(doc.files[1].hunks.size(), 1u);
}

// --- Binary file detection --------------------------------------------------

TEST(UnifiedDiffParser, BinaryFileNotice) {
  const std::string input = "Binary files a/image.png and b/image.png differ\n";

  const auto doc = parse_unified_diff(input);

  ASSERT_EQ(doc.files.size(), 1u);
  EXPECT_TRUE(doc.files[0].new_is_binary);
  EXPECT_EQ(doc.files[0].old_path, "image.png");
  EXPECT_EQ(doc.files[0].new_path, "image.png");
}

// --- /dev/null handling (new file, deleted file) ----------------------------

TEST(UnifiedDiffParser, NewFileFromDevNull) {
  const std::string input =
      "--- /dev/null\n"
      "+++ b/newfile.txt\n"
      "@@ -0,0 +1,2 @@\n"
      "+line one\n"
      "+line two\n";

  const auto doc = parse_unified_diff(input);

  ASSERT_EQ(doc.files.size(), 1u);
  EXPECT_EQ(doc.files[0].old_path, "/dev/null");
  EXPECT_EQ(doc.files[0].new_path, "newfile.txt");
  EXPECT_EQ(doc.files[0].hunks.size(), 1u);
  EXPECT_EQ(doc.files[0].hunks[0].lines.size(), 2u);
}

// --- a/ b/ prefix stripping -------------------------------------------------

TEST(UnifiedDiffParser, StripsAGitPrefix) {
  const std::string input =
      "--- a/src/foo.cpp\n"
      "+++ b/src/foo.cpp\n"
      "@@ -1 +1 @@\n"
      "-old\n"
      "+new\n";

  const auto doc = parse_unified_diff(input);

  ASSERT_EQ(doc.files.size(), 1u);
  EXPECT_EQ(doc.files[0].old_path, "src/foo.cpp");
  EXPECT_EQ(doc.files[0].new_path, "src/foo.cpp");
}

// --- DiffDocument helpers ---------------------------------------------------

TEST(UnifiedDiffParser, TotalLineCount) {
  const std::string input =
      "--- a/f.txt\n"
      "+++ b/f.txt\n"
      "@@ -1,2 +1,3 @@\n"
      " ctx\n"
      "-old\n"
      "+new\n"
      "+extra\n";

  const auto doc = parse_unified_diff(input);
  EXPECT_EQ(doc.total_line_count(), 4u);
}

TEST(UnifiedDiffParser, LineCounts) {
  const std::string input =
      "--- a/f.txt\n"
      "+++ b/f.txt\n"
      "@@ -1,3 +1,3 @@\n"
      " ctx\n"
      "-old\n"
      "+new\n";

  const auto doc = parse_unified_diff(input);
  const auto counts = doc.line_counts();
  EXPECT_EQ(counts.context, 1u);
  EXPECT_EQ(counts.additions, 1u);
  EXPECT_EQ(counts.deletions, 1u);
}

// --- Malformed / partial input ----------------------------------------------

TEST(UnifiedDiffParser, MalformedHunkHeaderSkipped) {
  // Hunk header without proper range
  const std::string input =
      "@@ garbage @@\n"
      " line\n";

  const auto doc = parse_unified_diff(input);
  // Garbage hunk header is skipped, no crash.
  EXPECT_EQ(doc.files.size(), 1u);
}

TEST(UnifiedDiffParser, HunkWithoutFileHeader) {
  // Hunk appearing without a --- / +++ header.
  const std::string input =
      "@@ -1 +1 @@\n"
      "-old\n"
      "+new\n";

  const auto doc = parse_unified_diff(input);
  ASSERT_EQ(doc.files.size(), 1u);
  EXPECT_EQ(doc.files[0].old_path, "");
  EXPECT_EQ(doc.files[0].new_path, "");
  ASSERT_EQ(doc.files[0].hunks.size(), 1u);
  EXPECT_EQ(doc.files[0].hunks[0].lines.size(), 2u);
}

// --- Rename handling --------------------------------------------------------

TEST(UnifiedDiffParser, RenameFile) {
  const std::string input =
      "rename from old_name.txt\n"
      "rename to new_name.txt\n"
      "--- a/old_name.txt\n"
      "+++ b/new_name.txt\n"
      "@@ -1 +1 @@\n"
      "-old\n"
      "+new\n";

  const auto doc = parse_unified_diff(input);
  ASSERT_EQ(doc.files.size(), 1u);
  EXPECT_TRUE(doc.files[0].is_rename);
  EXPECT_EQ(doc.files[0].old_path, "old_name.txt");
  EXPECT_EQ(doc.files[0].new_path, "new_name.txt");
}

// --- CRLF handling ----------------------------------------------------------

TEST(UnifiedDiffParser, HandlesCRLF) {
  const std::string input =
      "--- a/file.txt\r\n"
      "+++ b/file.txt\r\n"
      "@@ -1 +1 @@\r\n"
      "-old\r\n"
      "+new\r\n";

  const auto doc = parse_unified_diff(input);

  ASSERT_EQ(doc.files.size(), 1u);
  ASSERT_EQ(doc.files[0].hunks.size(), 1u);
  EXPECT_EQ(doc.files[0].hunks[0].lines.size(), 2u);
  EXPECT_EQ(doc.files[0].hunks[0].lines[0].text, "old");
  EXPECT_EQ(doc.files[0].hunks[0].lines[1].text, "new");
}

// --- File deletion: +++ /dev/null (file removal) ---------------------------

TEST(UnifiedDiffParser, DeletedFileToDevNull) {
  const std::string input =
      "--- a/oldfile.txt\n"
      "+++ /dev/null\n"
      "@@ -1,2 +0,0 @@\n"
      "-line one\n"
      "-line two\n";

  const auto doc = parse_unified_diff(input);

  ASSERT_EQ(doc.files.size(), 1u);
  EXPECT_EQ(doc.files[0].old_path, "oldfile.txt");
  EXPECT_EQ(doc.files[0].new_path, "oldfile.txt");
  EXPECT_FALSE(doc.files[0].new_is_binary);
  ASSERT_EQ(doc.files[0].hunks.size(), 1u);
  EXPECT_EQ(doc.files[0].hunks[0].lines.size(), 2u);
  EXPECT_EQ(doc.files[0].hunks[0].lines[0].kind, DiffLineKind::kDeletion);
  EXPECT_EQ(doc.files[0].hunks[0].lines[1].kind, DiffLineKind::kDeletion);
}

}  // namespace
}  // namespace terminal_ui_kit
