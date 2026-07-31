#include <string>
#include <cassert>

#include "terminal_ui_kit/diff/unified_diff_parser.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

// Helper function to safely get text from a diff line's spans.
// Asserts that the line has at least one span.
static std::string GetLineText(const DiffLine& line) {
  const auto& spans = line.content.spans();
  EXPECT_FALSE(spans.empty());
  if (spans.empty()) {
    return "";
  }
  return spans[0].text;
}

// Простой тест diff
static constexpr char kSimpleDiff[] =
    "--- original.txt\n"
    "+++ modified.txt\n"
    "@@ -1,3 +1,4 @@\n"
    " line one\n"
    "-old line two\n"
    "+new line two\n"
    " line three\n"
    "+line four\n";

TEST(UnifiedDiffParser, ParsesSimpleDiff) {
  UnifiedDiffParser parser;
  auto [success, file] = parser.Parse(kSimpleDiff);

  EXPECT_TRUE(success);
  EXPECT_EQ(file.old_path, "original.txt");
  EXPECT_EQ(file.new_path, "modified.txt");
  ASSERT_EQ(file.hunks.size(), 1U);

  const auto& hunk = file.hunks[0];
  EXPECT_EQ(hunk.header, "@@ -1,3 +1,4 @@");
  ASSERT_EQ(hunk.lines.size(), 5U);

  EXPECT_EQ(hunk.lines[0].type, DiffLineType::kContext);
  EXPECT_EQ(GetLineText(hunk.lines[0]), " line one");

  EXPECT_EQ(hunk.lines[1].type, DiffLineType::kDeletion);
  EXPECT_EQ(GetLineText(hunk.lines[1]), "old line two");

  EXPECT_EQ(hunk.lines[2].type, DiffLineType::kAddition);
  EXPECT_EQ(GetLineText(hunk.lines[2]), "new line two");

  EXPECT_EQ(hunk.lines[3].type, DiffLineType::kContext);
  EXPECT_EQ(GetLineText(hunk.lines[3]), " line three");

  EXPECT_EQ(hunk.lines[4].type, DiffLineType::kAddition);
  EXPECT_EQ(GetLineText(hunk.lines[4]), "line four");
}

TEST(UnifiedDiffParser, HandlesEmptyInput) {
  UnifiedDiffParser parser;
  auto [success, file] = parser.Parse("");

  EXPECT_FALSE(success);
}

TEST(UnifiedDiffParser, HandlesDiffWithoutIndexLine) {
  static constexpr char kDiffNoIndex[] =
      "--- a/file.txt\n"
      "+++ b/file.txt\n"
      "@@ -0,0 +1 @@\n"
      "+new content\n";

  UnifiedDiffParser parser;
  auto [success, file] = parser.Parse(kDiffNoIndex);

  EXPECT_TRUE(success);
  // a/ and b/ prefixes should be removed
  EXPECT_EQ(file.old_path, "file.txt");
  EXPECT_EQ(file.new_path, "file.txt");
  ASSERT_EQ(file.hunks.size(), 1U);
  ASSERT_EQ(file.hunks[0].lines.size(), 1U);
  EXPECT_EQ(file.hunks[0].lines[0].type, DiffLineType::kAddition);
}

TEST(UnifiedDiffParser, ParsesMultipleHunks) {
  static constexpr char kMultiHunkDiff[] =
      "--- original.txt\n"
      "+++ modified.txt\n"
      "@@ -1,2 +1,2 @@\n"
      " line one\n"
      "-old\n"
      "+new\n"
      "@@ -10,2 +10,2 @@\n"
      " another line\n"
      "-deleted\n"
      "+added\n";

  UnifiedDiffParser parser;
  auto [success, file] = parser.Parse(kMultiHunkDiff);

  EXPECT_TRUE(success);
  ASSERT_EQ(file.hunks.size(), 2U);
  EXPECT_EQ(file.hunks[0].header, "@@ -1,2 +1,2 @@");
  EXPECT_EQ(file.hunks[1].header, "@@ -10,2 +10,2 @@");
}

TEST(UnifiedDiffParser, HandlesNoNewlineAtEnd) {
  static constexpr char kNoNewlineDiff[] =
      "--- original.txt\n"
      "+++ modified.txt\n"
      "@@ -1,2 +1,2 @@\n"
      " line one\n"
      "-line two\n"
      "\\ No newline at end of file\n";

  UnifiedDiffParser parser;
  auto [success, file] = parser.Parse(kNoNewlineDiff);

  EXPECT_TRUE(success);
  ASSERT_EQ(file.hunks.size(), 1U);
  ASSERT_GE(file.hunks[0].lines.size(), 2U);
  EXPECT_EQ(file.hunks[0].lines[0].type, DiffLineType::kContext);
  EXPECT_EQ(file.hunks[0].lines[1].type, DiffLineType::kDeletion);
}

TEST(UnifiedDiffParser, HandlesPlusPrefixForAddition) {
  static constexpr char kPlusDiff[] =
      "--- original.txt\n"
      "+++ modified.txt\n"
      "@@ -1 +1,2 @@\n"
      "+added line\n"
      "+another line\n";

  UnifiedDiffParser parser;
  auto [success, file] = parser.Parse(kPlusDiff);

  EXPECT_TRUE(success);
  ASSERT_EQ(file.hunks.size(), 1U);
  ASSERT_EQ(file.hunks[0].lines.size(), 2U);
  EXPECT_EQ(file.hunks[0].lines[0].type, DiffLineType::kAddition);
  EXPECT_EQ(file.hunks[0].lines[1].type, DiffLineType::kAddition);
}

TEST(UnifiedDiffParser, HandlesMinusPrefixForDeletion) {
  static constexpr char kMinusDiff[] =
      "--- original.txt\n"
      "+++ /dev/null\n"
      "@@ -1,2 +0,0 @@\n"
      "-deleted line one\n"
      "-deleted line two\n";

  UnifiedDiffParser parser;
  auto [success, file] = parser.Parse(kMinusDiff);

  EXPECT_TRUE(success);
  ASSERT_EQ(file.hunks.size(), 1U);
  ASSERT_EQ(file.hunks[0].lines.size(), 2U);
  EXPECT_EQ(file.hunks[0].lines[0].type, DiffLineType::kDeletion);
  EXPECT_EQ(file.hunks[0].lines[1].type, DiffLineType::kDeletion);
}

TEST(UnifiedDiffParser, HandlesOnlyContextLines) {
  static constexpr char kContextOnlyDiff[] =
      "--- original.txt\n"
      "+++ modified.txt\n"
      "@@ -1,2 +1,2 @@\n"
      " same line one\n"
      " same line two\n";

  UnifiedDiffParser parser;
  auto [success, file] = parser.Parse(kContextOnlyDiff);

  EXPECT_TRUE(success);
  ASSERT_EQ(file.hunks.size(), 1U);
  ASSERT_EQ(file.hunks[0].lines.size(), 2U);
  EXPECT_EQ(file.hunks[0].lines[0].type, DiffLineType::kContext);
  EXPECT_EQ(file.hunks[0].lines[1].type, DiffLineType::kContext);
}

}  // namespace
}  // namespace terminal_ui_kit