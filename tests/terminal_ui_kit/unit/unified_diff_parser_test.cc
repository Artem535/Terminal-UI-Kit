#include "terminal_ui_kit/diff/unified_diff_parser.h"

#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace terminal_ui_kit::diff {
namespace {

std::string Content(const DiffLine& line) {
  std::string result;
  for (const TextSpan& span : line.content.spans()) {
    result += span.text;
  }
  return result;
}

TEST(UnifiedDiffParser, ParsesSingleFileWithHunk) {
  const std::string_view text =
      "diff --git a/foo.txt b/foo.txt\n"
      "index 1234567..89abcde 100644\n"
      "--- a/foo.txt\n"
      "+++ b/foo.txt\n"
      "@@ -1,3 +1,3 @@\n"
      " context\n"
      "-old line\n"
      "+new line\n"
      " context2\n";

  DiffDocument document = parse_unified_diff(text);

  ASSERT_EQ(document.files.size(), 1u);
  const DiffFile& file = document.files[0];
  EXPECT_EQ(file.old_path, "foo.txt");
  EXPECT_EQ(file.new_path, "foo.txt");
  EXPECT_FALSE(file.is_binary);

  ASSERT_EQ(file.hunks.size(), 1u);
  const DiffHunk& hunk = file.hunks[0];
  EXPECT_EQ(hunk.header, "@@ -1,3 +1,3 @@");

  ASSERT_EQ(hunk.lines.size(), 4u);
  EXPECT_EQ(hunk.lines[0].type, DiffLineType::kContext);
  EXPECT_EQ(Content(hunk.lines[0]), "context");
  ASSERT_TRUE(hunk.lines[0].old_line.has_value());
  ASSERT_TRUE(hunk.lines[0].new_line.has_value());
  EXPECT_EQ(*hunk.lines[0].old_line, 1);
  EXPECT_EQ(*hunk.lines[0].new_line, 1);

  EXPECT_EQ(hunk.lines[1].type, DiffLineType::kDeletion);
  EXPECT_EQ(Content(hunk.lines[1]), "old line");
  ASSERT_TRUE(hunk.lines[1].old_line.has_value());
  EXPECT_EQ(*hunk.lines[1].old_line, 2);
  EXPECT_FALSE(hunk.lines[1].new_line.has_value());

  EXPECT_EQ(hunk.lines[2].type, DiffLineType::kAddition);
  EXPECT_EQ(Content(hunk.lines[2]), "new line");
  EXPECT_FALSE(hunk.lines[2].old_line.has_value());
  ASSERT_TRUE(hunk.lines[2].new_line.has_value());
  EXPECT_EQ(*hunk.lines[2].new_line, 2);

  EXPECT_EQ(hunk.lines[3].type, DiffLineType::kContext);
  EXPECT_EQ(Content(hunk.lines[3]), "context2");
  ASSERT_TRUE(hunk.lines[3].old_line.has_value());
  ASSERT_TRUE(hunk.lines[3].new_line.has_value());
  EXPECT_EQ(*hunk.lines[3].old_line, 3);
  EXPECT_EQ(*hunk.lines[3].new_line, 3);
}

TEST(UnifiedDiffParser, ParsesMultipleFiles) {
  const std::string_view text =
      "diff --git a/a.txt b/a.txt\n"
      "--- a/a.txt\n"
      "+++ b/a.txt\n"
      "@@ -1 +1 @@\n"
      "-a\n"
      "+b\n"
      "diff --git a/c.txt b/c.txt\n"
      "--- a/c.txt\n"
      "+++ b/c.txt\n"
      "@@ -1 +1 @@\n"
      "-c\n"
      "+d\n";

  DiffDocument document = parse_unified_diff(text);

  ASSERT_EQ(document.files.size(), 2u);
  EXPECT_EQ(document.files[0].old_path, "a.txt");
  EXPECT_EQ(document.files[1].old_path, "c.txt");
  EXPECT_EQ(document.files[0].hunks.size(), 1u);
  EXPECT_EQ(document.files[1].hunks.size(), 1u);
}

TEST(UnifiedDiffParser, HandlesEmptyInput) {
  DiffDocument document = parse_unified_diff("");
  EXPECT_TRUE(document.files.empty());
}

TEST(UnifiedDiffParser, HandlesEmptyFile) {
  // A diff for an empty file has no hunks.
  const std::string_view text =
      "diff --git a/empty.txt b/empty.txt\n"
      "new file mode 100644\n"
      "index 0000000..e69de29\n"
      "--- /dev/null\n"
      "+++ b/empty.txt\n";

  DiffDocument document = parse_unified_diff(text);

  ASSERT_EQ(document.files.size(), 1u);
  EXPECT_EQ(document.files[0].old_path, "/dev/null");
  EXPECT_EQ(document.files[0].new_path, "empty.txt");
  EXPECT_TRUE(document.files[0].hunks.empty());
}

TEST(UnifiedDiffParser, HandlesBinaryFileNotice) {
  const std::string_view text =
      "diff --git a/img.png b/img.png\n"
      "index 1234567..89abcde 100644\n"
      "Binary files a/img.png and b/img.png differ\n";

  DiffDocument document = parse_unified_diff(text);

  ASSERT_EQ(document.files.size(), 1u);
  EXPECT_TRUE(document.files[0].is_binary);
  EXPECT_TRUE(document.files[0].hunks.empty());
}

TEST(UnifiedDiffParser, HandlesMalformedHunkHeader) {
  const std::string_view text =
      "diff --git a/foo.txt b/foo.txt\n"
      "--- a/foo.txt\n"
      "+++ b/foo.txt\n"
      "@@ not a valid header @@\n"
      "+line\n";

  DiffDocument document = parse_unified_diff(text);

  ASSERT_EQ(document.files.size(), 1u);
  EXPECT_TRUE(document.files[0].hunks.empty());
}

TEST(UnifiedDiffParser, HandlesMalformedHunkHeaderWithMissingPlus) {
  const std::string_view text =
      "diff --git a/foo.txt b/foo.txt\n"
      "--- a/foo.txt\n"
      "+++ b/foo.txt\n"
      "@@ -1,3 @@\n"
      "+line\n";

  DiffDocument document = parse_unified_diff(text);

  ASSERT_EQ(document.files.size(), 1u);
  EXPECT_TRUE(document.files[0].hunks.empty());
}

TEST(UnifiedDiffParser, HandlesNoNewlineAtEndOfFileMarker) {
  const std::string_view text =
      "diff --git a/foo.txt b/foo.txt\n"
      "--- a/foo.txt\n"
      "+++ b/foo.txt\n"
      "@@ -1 +1 @@\n"
      "-old\n"
      "\\ No newline at end of file\n"
      "+new\n"
      "\\ No newline at end of file\n";

  DiffDocument document = parse_unified_diff(text);

  ASSERT_EQ(document.files.size(), 1u);
  ASSERT_EQ(document.files[0].hunks.size(), 1u);
  // The "\ No newline" markers must not become lines.
  EXPECT_EQ(document.files[0].hunks[0].lines.size(), 2u);
}

TEST(UnifiedDiffParser, HandlesCrlfLineEndings) {
  const std::string_view text =
      "diff --git a/foo.txt b/foo.txt\r\n"
      "--- a/foo.txt\r\n"
      "+++ b/foo.txt\r\n"
      "@@ -1 +1 @@\r\n"
      "-old\r\n"
      "+new\r\n";

  DiffDocument document = parse_unified_diff(text);

  ASSERT_EQ(document.files.size(), 1u);
  ASSERT_EQ(document.files[0].hunks.size(), 1u);
  ASSERT_EQ(document.files[0].hunks[0].lines.size(), 2u);
  EXPECT_EQ(Content(document.files[0].hunks[0].lines[0]), "old");
  EXPECT_EQ(Content(document.files[0].hunks[0].lines[1]), "new");
}

TEST(UnifiedDiffParser, StripsTimestampFromPaths) {
  const std::string_view text =
      "--- a/foo.txt\t2024-01-01 12:00:00.000000000 +0000\n"
      "+++ b/foo.txt\t2024-01-01 12:00:01.000000000 +0000\n"
      "@@ -1 +1 @@\n"
      "-old\n"
      "+new\n";

  DiffDocument document = parse_unified_diff(text);

  ASSERT_EQ(document.files.size(), 1u);
  EXPECT_EQ(document.files[0].old_path, "foo.txt");
  EXPECT_EQ(document.files[0].new_path, "foo.txt");
}

TEST(UnifiedDiffParser, HandlesRenameWithoutHunks) {
  const std::string_view text =
      "diff --git a/old.txt b/new.txt\n"
      "similarity index 100%\n"
      "rename from old.txt\n"
      "rename to new.txt\n";

  DiffDocument document = parse_unified_diff(text);

  ASSERT_EQ(document.files.size(), 1u);
  EXPECT_EQ(document.files[0].old_path, "old.txt");
  EXPECT_EQ(document.files[0].new_path, "new.txt");
  EXPECT_TRUE(document.files[0].hunks.empty());
}

TEST(UnifiedDiffParser, IgnoresUnrecognizedLinesOutsideHunks) {
  const std::string_view text =
      "some random preamble\n"
      "diff --git a/foo.txt b/foo.txt\n"
      "--- a/foo.txt\n"
      "+++ b/foo.txt\n"
      "@@ -1 +1 @@\n"
      "-old\n"
      "+new\n"
      "trailing garbage\n";

  DiffDocument document = parse_unified_diff(text);

  ASSERT_EQ(document.files.size(), 1u);
  ASSERT_EQ(document.files[0].hunks.size(), 1u);
  EXPECT_EQ(document.files[0].hunks[0].lines.size(), 2u);
}

TEST(UnifiedDiffParser, HandlesHunkWithZeroCounts) {
  const std::string_view text =
      "diff --git a/foo.txt b/foo.txt\n"
      "--- a/foo.txt\n"
      "+++ b/foo.txt\n"
      "@@ -0,0 +1,2 @@\n"
      "+new1\n"
      "+new2\n";

  DiffDocument document = parse_unified_diff(text);

  ASSERT_EQ(document.files.size(), 1u);
  ASSERT_EQ(document.files[0].hunks.size(), 1u);
  ASSERT_EQ(document.files[0].hunks[0].lines.size(), 2u);
  EXPECT_EQ(document.files[0].hunks[0].lines[0].type, DiffLineType::kAddition);
  ASSERT_TRUE(document.files[0].hunks[0].lines[0].new_line.has_value());
  EXPECT_EQ(*document.files[0].hunks[0].lines[0].new_line, 1);
  EXPECT_FALSE(document.files[0].hunks[0].lines[0].old_line.has_value());
}

TEST(UnifiedDiffParser, HandlesMultipleHunksInOneFile) {
  const std::string_view text =
      "diff --git a/foo.txt b/foo.txt\n"
      "--- a/foo.txt\n"
      "+++ b/foo.txt\n"
      "@@ -1,2 +1,2 @@\n"
      " a\n"
      "-b\n"
      "+B\n"
      "@@ -10,2 +10,2 @@\n"
      " j\n"
      "-k\n"
      "+K\n";

  DiffDocument document = parse_unified_diff(text);

  ASSERT_EQ(document.files.size(), 1u);
  ASSERT_EQ(document.files[0].hunks.size(), 2u);
  EXPECT_EQ(document.files[0].hunks[0].header, "@@ -1,2 +1,2 @@");
  EXPECT_EQ(document.files[0].hunks[1].header, "@@ -10,2 +10,2 @@");
  EXPECT_EQ(document.files[0].hunks[1].lines.size(), 3u);
}

TEST(UnifiedDiffParser, HandlesContentLinesStartingWithDiffMarker) {
  // A context line whose content begins with "diff --git" must stay inside
  // the hunk, not start a new file.
  const std::string_view text =
      "diff --git a/foo.txt b/foo.txt\n"
      "--- a/foo.txt\n"
      "+++ b/foo.txt\n"
      "@@ -1 +1 @@\n"
      " diff --git is just text here\n";

  DiffDocument document = parse_unified_diff(text);

  ASSERT_EQ(document.files.size(), 1u);
  ASSERT_EQ(document.files[0].hunks.size(), 1u);
  ASSERT_EQ(document.files[0].hunks[0].lines.size(), 1u);
  EXPECT_EQ(Content(document.files[0].hunks[0].lines[0]), "diff --git is just text here");
}

TEST(UnifiedDiffParser, HandlesDeletionOnlyHunk) {
  // A pure-deletion hunk has new_start 0 (file truncation).
  const std::string_view text =
      "diff --git a/foo.txt b/foo.txt\n"
      "--- a/foo.txt\n"
      "+++ b/foo.txt\n"
      "@@ -1,2 +0,0 @@\n"
      "-gone1\n"
      "-gone2\n";

  DiffDocument document = parse_unified_diff(text);

  ASSERT_EQ(document.files.size(), 1u);
  ASSERT_EQ(document.files[0].hunks.size(), 1u);
  ASSERT_EQ(document.files[0].hunks[0].lines.size(), 2u);
  EXPECT_EQ(document.files[0].hunks[0].lines[0].type, DiffLineType::kDeletion);
  ASSERT_TRUE(document.files[0].hunks[0].lines[0].old_line.has_value());
  EXPECT_EQ(*document.files[0].hunks[0].lines[0].old_line, 1);
  EXPECT_FALSE(document.files[0].hunks[0].lines[0].new_line.has_value());
  EXPECT_EQ(document.files[0].hunks[0].lines[1].type, DiffLineType::kDeletion);
  ASSERT_TRUE(document.files[0].hunks[0].lines[1].old_line.has_value());
  EXPECT_EQ(*document.files[0].hunks[0].lines[1].old_line, 2);
}

TEST(UnifiedDiffParser, HandlesHunkHeaderWithFunctionContext) {
  // git diff appends a function name after the second "@@".
  const std::string_view text =
      "diff --git a/foo.cc b/foo.cc\n"
      "--- a/foo.cc\n"
      "+++ b/foo.cc\n"
      "@@ -1,3 +1,3 @@ int main() {\n"
      " int a;\n"
      "-int b;\n"
      "+int c;\n"
      " return 0;\n";

  DiffDocument document = parse_unified_diff(text);

  ASSERT_EQ(document.files.size(), 1u);
  ASSERT_EQ(document.files[0].hunks.size(), 1u);
  EXPECT_EQ(document.files[0].hunks[0].header, "@@ -1,3 +1,3 @@ int main() {");
  ASSERT_EQ(document.files[0].hunks[0].lines.size(), 4u);
  EXPECT_EQ(document.files[0].hunks[0].lines[0].type, DiffLineType::kContext);
  EXPECT_EQ(document.files[0].hunks[0].lines[1].type, DiffLineType::kDeletion);
  EXPECT_EQ(document.files[0].hunks[0].lines[2].type, DiffLineType::kAddition);
  EXPECT_EQ(document.files[0].hunks[0].lines[3].type, DiffLineType::kContext);
}

TEST(UnifiedDiffParser, HandlesTruncatedHunk) {
  // The header declares more lines than are present; the parser keeps the
  // lines that do appear and does not crash.
  const std::string_view text =
      "diff --git a/foo.txt b/foo.txt\n"
      "--- a/foo.txt\n"
      "+++ b/foo.txt\n"
      "@@ -1,5 +1,5 @@\n"
      " a\n"
      "-b\n";

  DiffDocument document = parse_unified_diff(text);

  ASSERT_EQ(document.files.size(), 1u);
  ASSERT_EQ(document.files[0].hunks.size(), 1u);
  ASSERT_EQ(document.files[0].hunks[0].lines.size(), 2u);
}

TEST(UnifiedDiffParser, HandlesCopyWithoutHunks) {
  const std::string_view text =
      "diff --git a/src.txt b/dst.txt\n"
      "similarity index 100%\n"
      "copy from src.txt\n"
      "copy to dst.txt\n";

  DiffDocument document = parse_unified_diff(text);

  ASSERT_EQ(document.files.size(), 1u);
  EXPECT_EQ(document.files[0].old_path, "src.txt");
  EXPECT_EQ(document.files[0].new_path, "dst.txt");
  EXPECT_TRUE(document.files[0].hunks.empty());
}

TEST(UnifiedDiffParser, HandlesNewFileWithContent) {
  const std::string_view text =
      "diff --git a/new.txt b/new.txt\n"
      "new file mode 100644\n"
      "index 0000000..e69de29\n"
      "--- /dev/null\n"
      "+++ b/new.txt\n"
      "@@ -0,0 +1,2 @@\n"
      "+hello\n"
      "+world\n";

  DiffDocument document = parse_unified_diff(text);

  ASSERT_EQ(document.files.size(), 1u);
  EXPECT_EQ(document.files[0].old_path, "/dev/null");
  EXPECT_EQ(document.files[0].new_path, "new.txt");
  ASSERT_EQ(document.files[0].hunks.size(), 1u);
  ASSERT_EQ(document.files[0].hunks[0].lines.size(), 2u);
  EXPECT_EQ(document.files[0].hunks[0].lines[0].type, DiffLineType::kAddition);
  EXPECT_EQ(Content(document.files[0].hunks[0].lines[0]), "hello");
  EXPECT_EQ(Content(document.files[0].hunks[0].lines[1]), "world");
}

TEST(UnifiedDiffParser, RejectsOverflowingLineNumbers) {
  // An absurdly long line number must not overflow int (undefined behavior);
  // the malformed hunk header is skipped predictably.
  const std::string_view text =
      "diff --git a/foo.txt b/foo.txt\n"
      "--- a/foo.txt\n"
      "+++ b/foo.txt\n"
      "@@ -99999999999999999999 +1 @@\n"
      "+line\n";

  DiffDocument document = parse_unified_diff(text);

  ASSERT_EQ(document.files.size(), 1u);
  EXPECT_TRUE(document.files[0].hunks.empty());
}

}  // namespace
}  // namespace terminal_ui_kit::diff
