#include "terminal_ui_kit/diff/unified_diff_parser.h"

#include <string>
#include <string_view>

#include "terminal_ui_kit/core/styled_text.h"
#include "terminal_ui_kit/core/text_style.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

std::string plain_text(const StyledText& text) {
  std::string result;
  for (const auto& span : text.spans()) {
    result += span.text;
  }
  return result;
}

TEST(UnifiedDiffParser, EmptyInput) {
  auto result = parse_unified_diff("");
  EXPECT_TRUE(result.success);
  EXPECT_TRUE(result.model.empty());
}

TEST(UnifiedDiffParser, SingleHunk) {
  std::string diff =
      "--- a/file.txt\n"
      "+++ b/file.txt\n"
      "@@ -1,3 +1,3 @@\n"
      " context before\n"
      "-removed line\n"
      "+added line\n"
      " context after\n";

  auto result = parse_unified_diff(diff);
  ASSERT_TRUE(result.success);
  ASSERT_EQ(result.model.file_count(), 1u);

  const auto& file = result.model.file_at(0);
  EXPECT_EQ(file.old_path, "a/file.txt");
  EXPECT_EQ(file.new_path, "b/file.txt");
  EXPECT_FALSE(file.is_new_file);
  EXPECT_FALSE(file.is_deleted_file);
  EXPECT_FALSE(file.is_binary);
  ASSERT_EQ(file.hunks.size(), 1u);

  const auto& hunk = file.hunks[0];
  EXPECT_EQ(hunk.old_start, 1);
  EXPECT_EQ(hunk.old_count, 3);
  EXPECT_EQ(hunk.new_start, 1);
  EXPECT_EQ(hunk.new_count, 3);

  ASSERT_EQ(hunk.lines.size(), 4u);
  EXPECT_EQ(hunk.lines[0].type, DiffLineType::kContext);
  EXPECT_EQ(hunk.lines[0].old_line, 1);
  EXPECT_EQ(hunk.lines[0].new_line, 1);
  EXPECT_EQ(plain_text(hunk.lines[0].content), "context before");

  EXPECT_EQ(hunk.lines[1].type, DiffLineType::kDeletion);
  EXPECT_EQ(hunk.lines[1].old_line, 2);
  EXPECT_EQ(hunk.lines[1].new_line, std::nullopt);
  EXPECT_EQ(plain_text(hunk.lines[1].content), "removed line");

  EXPECT_EQ(hunk.lines[2].type, DiffLineType::kAddition);
  EXPECT_EQ(hunk.lines[2].old_line, std::nullopt);
  EXPECT_EQ(hunk.lines[2].new_line, 2);
  EXPECT_EQ(plain_text(hunk.lines[2].content), "added line");

  EXPECT_EQ(hunk.lines[3].type, DiffLineType::kContext);
  EXPECT_EQ(hunk.lines[3].old_line, 3);
  EXPECT_EQ(hunk.lines[3].new_line, 3);
  EXPECT_EQ(plain_text(hunk.lines[3].content), "context after");
}

TEST(UnifiedDiffParser, NewFile) {
  std::string diff =
      "--- /dev/null\n"
      "+++ b/new_file.txt\n"
      "@@ -0,0 +1,2 @@\n"
      "+line one\n"
      "+line two\n";

  auto result = parse_unified_diff(diff);
  ASSERT_TRUE(result.success);
  ASSERT_EQ(result.model.file_count(), 1u);
  const auto& file = result.model.file_at(0);
  EXPECT_EQ(file.old_path, "/dev/null");
  EXPECT_EQ(file.new_path, "b/new_file.txt");
  EXPECT_TRUE(file.is_new_file);
  EXPECT_FALSE(file.is_deleted_file);
  ASSERT_EQ(file.hunks.size(), 1u);
  EXPECT_EQ(file.hunks[0].new_start, 1);
}

TEST(UnifiedDiffParser, DeletedFile) {
  std::string diff =
      "--- a/old_file.txt\n"
      "+++ /dev/null\n"
      "@@ -1,2 +0,0 @@\n"
      "-line one\n"
      "-line two\n";

  auto result = parse_unified_diff(diff);
  ASSERT_TRUE(result.success);
  ASSERT_EQ(result.model.file_count(), 1u);
  const auto& file = result.model.file_at(0);
  EXPECT_EQ(file.old_path, "a/old_file.txt");
  EXPECT_EQ(file.new_path, "/dev/null");
  EXPECT_TRUE(file.is_deleted_file);
  EXPECT_FALSE(file.is_new_file);
}

TEST(UnifiedDiffParser, MultipleFiles) {
  std::string diff =
      "--- a/first.txt\n"
      "+++ b/first.txt\n"
      "@@ -1,1 +1,1 @@\n"
      "-old\n"
      "+new\n"
      "--- a/second.txt\n"
      "+++ b/second.txt\n"
      "@@ -2,1 +2,1 @@\n"
      "-x\n"
      "+y\n";

  auto result = parse_unified_diff(diff);
  ASSERT_TRUE(result.success);
  ASSERT_EQ(result.model.file_count(), 2u);
  EXPECT_EQ(result.model.file_at(0).old_path, "a/first.txt");
  EXPECT_EQ(result.model.file_at(1).old_path, "a/second.txt");
}

TEST(UnifiedDiffParser, MultipleHunks) {
  std::string diff =
      "--- a/file.txt\n"
      "+++ b/file.txt\n"
      "@@ -1,2 +1,2 @@\n"
      "-a\n"
      "+b\n"
      " context\n"
      "@@ -10,2 +10,2 @@\n"
      "-c\n"
      "+d\n"
      " more context\n";

  auto result = parse_unified_diff(diff);
  ASSERT_TRUE(result.success);
  ASSERT_EQ(result.model.file_count(), 1u);
  const auto& file = result.model.file_at(0);
  ASSERT_EQ(file.hunks.size(), 2u);
  EXPECT_EQ(file.hunks[0].old_start, 1);
  EXPECT_EQ(file.hunks[1].old_start, 10);
}

TEST(UnifiedDiffParser, BinaryFileNotice) {
  std::string diff =
      "--- a/image.png\n"
      "+++ b/image.png\n"
      "Binary files differ\n";

  auto result = parse_unified_diff(diff);
  ASSERT_TRUE(result.success);
  ASSERT_EQ(result.model.file_count(), 1u);
  const auto& file = result.model.file_at(0);
  EXPECT_TRUE(file.is_binary);
  EXPECT_TRUE(file.hunks.empty());
}

TEST(UnifiedDiffParser, IgnoresIndexAndGitHeader) {
  std::string diff =
      "diff --git a/file.txt b/file.txt\n"
      "index 1234567..abcdefg 100644\n"
      "--- a/file.txt\n"
      "+++ b/file.txt\n"
      "@@ -1,1 +1,1 @@\n"
      "-old\n"
      "+new\n";

  auto result = parse_unified_diff(diff);
  ASSERT_TRUE(result.success);
  ASSERT_EQ(result.model.file_count(), 1u);
  EXPECT_EQ(result.model.file_at(0).hunks.size(), 1u);
}

TEST(UnifiedDiffParser, HandlesNoNewlineAtEnd) {
  std::string diff =
      "--- a/file.txt\n"
      "+++ b/file.txt\n"
      "@@ -1,1 +1,1 @@\n"
      "-old\n"
      "+new\n"
      "\\ No newline at end of file\n";

  auto result = parse_unified_diff(diff);
  ASSERT_TRUE(result.success);
  ASSERT_EQ(result.model.file_count(), 1u);
}

TEST(UnifiedDiffParser, MalformedHunkHeader) {
  std::string diff =
      "--- a/file.txt\n"
      "+++ b/file.txt\n"
      "@@ malformed @@\n"
      " context\n";

  auto result = parse_unified_diff(diff);
  EXPECT_FALSE(result.success);
  EXPECT_NE(result.error_message.find("Malformed"), std::string::npos);
}

TEST(UnifiedDiffParser, ContextLineOutsideHunk) {
  std::string diff =
      "--- a/file.txt\n"
      "+++ b/file.txt\n"
      " context without hunk\n";

  auto result = parse_unified_diff(diff);
  EXPECT_FALSE(result.success);
  EXPECT_NE(result.error_message.find("outside of hunk"), std::string::npos);
}

TEST(UnifiedDiffParser, CRLFLineEndings) {
  std::string diff =
      "--- a/file.txt\r\n"
      "+++ b/file.txt\r\n"
      "@@ -1,1 +1,1 @@\r\n"
      "-old\r\n"
      "+new\r\n";

  auto result = parse_unified_diff(diff);
  ASSERT_TRUE(result.success);
  ASSERT_EQ(result.model.file_count(), 1u);
  const auto& file = result.model.file_at(0);
  ASSERT_EQ(file.hunks.size(), 1u);
  ASSERT_EQ(file.hunks[0].lines.size(), 2u);
  EXPECT_EQ(plain_text(file.hunks[0].lines[0].content), "old");
  EXPECT_EQ(plain_text(file.hunks[0].lines[1].content), "new");
}

TEST(UnifiedDiffParser, LeadingAndTrailingNoise) {
  std::string diff =
      "Some random text before diff\n"
      "--- a/file.txt\n"
      "+++ b/file.txt\n"
      "@@ -1,1 +1,1 @@\n"
      "-old\n"
      "+new\n"
      "Some random text after diff\n";

  auto result = parse_unified_diff(diff);
  ASSERT_TRUE(result.success);
  ASSERT_EQ(result.model.file_count(), 1u);
  EXPECT_EQ(result.model.file_at(0).new_path, "b/file.txt");
}

TEST(UnifiedDiffParser, EmptyFileNew) {
  std::string diff =
      "--- /dev/null\n"
      "+++ b/empty.txt\n"
      "@@ -0,0 +1 @@\n";

  auto result = parse_unified_diff(diff);
  ASSERT_TRUE(result.success);
  ASSERT_EQ(result.model.file_count(), 1u);
  const auto& file = result.model.file_at(0);
  EXPECT_TRUE(file.is_new_file);
  ASSERT_EQ(file.hunks.size(), 1u);
  EXPECT_TRUE(file.hunks[0].lines.empty());
}

TEST(UnifiedDiffParser, HeaderAfterTab) {
  std::string diff =
      "--- a/file.txt\t2024-01-01 00:00:00.000000000 +0000\n"
      "+++ b/file.txt\t2024-01-02 00:00:00.000000000 +0000\n"
      "@@ -1,1 +1,1 @@\n"
      "-old\n"
      "+new\n";

  auto result = parse_unified_diff(diff);
  ASSERT_TRUE(result.success);
  const auto& file = result.model.file_at(0);
  EXPECT_EQ(file.old_path, "a/file.txt");
  EXPECT_EQ(file.new_path, "b/file.txt");
}

TEST(UnifiedDiffParser, CopyRenameDetectionPaths) {
  std::string diff =
      "--- old_name.txt\n"
      "+++ new_name.txt\n"
      "@@ -1,1 +1,1 @@\n"
      "-a\n"
      "+b\n";

  auto result = parse_unified_diff(diff);
  ASSERT_TRUE(result.success);
  const auto& file = result.model.file_at(0);
  EXPECT_EQ(file.old_path, "old_name.txt");
  EXPECT_EQ(file.new_path, "new_name.txt");
}

TEST(UnifiedDiffParser, NoValidDiffContent) {
  std::string diff = "Just some random text without any diff markers.\n";
  auto result = parse_unified_diff(diff);
  EXPECT_FALSE(result.success);
  EXPECT_NE(result.error_message.find("No valid diff"), std::string::npos);
}

TEST(UnifiedDiffParser, HandlesNoTrailingNewline) {
  std::string diff =
      "--- a/file.txt\n"
      "+++ b/file.txt\n"
      "@@ -1,1 +1,1 @@\n"
      "-old\n"
      "+new";

  auto result = parse_unified_diff(diff);
  ASSERT_TRUE(result.success);
  ASSERT_EQ(result.model.file_count(), 1u);
  const auto& file = result.model.file_at(0);
  ASSERT_EQ(file.hunks.size(), 1u);
  ASSERT_EQ(file.hunks[0].lines.size(), 2u);
  EXPECT_EQ(plain_text(file.hunks[0].lines[1].content), "new");
}

}  // namespace
}  // namespace terminal_ui_kit
