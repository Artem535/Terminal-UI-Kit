#include "terminal_ui_kit/diff/unified_diff.h"

#include <gtest/gtest.h>

using namespace terminal_ui_kit;

TEST(UnifiedDiffParser, SimpleFileSingleHunk) {
  const std::string diff =
      "--- a/example.txt\n"
      "+++ b/example.txt\n"
      "@@ -1,3 +1,4 @@\n"
      " line1\n"
      "-line2\n"
      "+line2_modified\n"
      " line3\n"
      "+line4_new\n";

  auto files = UnifiedDiffParser::Parse(diff);
  ASSERT_EQ(files.size(), 1u);
  const auto& file = files[0];
  EXPECT_EQ(file.old_path, "a/example.txt");
  EXPECT_EQ(file.new_path, "b/example.txt");
  ASSERT_EQ(file.hunks.size(), 1u);
  const auto& hunk = file.hunks[0];
  EXPECT_EQ(hunk.old_start, 1u);
  EXPECT_EQ(hunk.old_lines, 3u);
  EXPECT_EQ(hunk.new_start, 1u);
  EXPECT_EQ(hunk.new_lines, 4u);
  // Expected line sequence: context, removed, added, context, added
  ASSERT_EQ(hunk.lines.size(), 5u);
  EXPECT_EQ(hunk.lines[0].type, DiffLine::Type::Context);
  EXPECT_EQ(hunk.lines[0].old_line, 1u);
  EXPECT_EQ(hunk.lines[0].new_line, 1u);
  EXPECT_EQ(hunk.lines[1].type, DiffLine::Type::Removed);
  EXPECT_EQ(hunk.lines[1].old_line, 2u);
  EXPECT_EQ(hunk.lines[1].new_line, std::nullopt);
  EXPECT_EQ(hunk.lines[2].type, DiffLine::Type::Added);
  EXPECT_EQ(hunk.lines[2].old_line, std::nullopt);
  EXPECT_EQ(hunk.lines[2].new_line, 2u);
  EXPECT_EQ(hunk.lines[3].type, DiffLine::Type::Context);
  EXPECT_EQ(hunk.lines[3].old_line, 3u);
  EXPECT_EQ(hunk.lines[3].new_line, 3u);
  EXPECT_EQ(hunk.lines[4].type, DiffLine::Type::Added);
  EXPECT_EQ(hunk.lines[4].old_line, std::nullopt);
  EXPECT_EQ(hunk.lines[4].new_line, 4u);
}

TEST(UnifiedDiffParser, BinaryFile) {
  const std::string diff =
      "--- a/binary.bin\n"
      "+++ b/binary.bin\n"
      "Binary files a/binary.bin and b/binary.bin differ\n";
  auto files = UnifiedDiffParser::Parse(diff);
  ASSERT_EQ(files.size(), 1u);
  EXPECT_TRUE(files[0].is_binary);
}
