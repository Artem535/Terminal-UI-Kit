#include "terminal_ui_kit/diff/unified_diff_parser.h"

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

// =========================================================================
// Basic parsing
// =========================================================================

TEST(ParseUnifiedDiff, EmptyInput) {
  auto result = parse_unified_diff("");
  EXPECT_TRUE(result.files.empty());
  EXPECT_FALSE(result.error_line.has_value());
}

TEST(ParseUnifiedDiff, SingleFileWithSingleHunk) {
  constexpr std::string_view kDiff = R"(--- a/hello.txt
+++ b/hello.txt
@@ -1,3 +1,3 @@
 hello
-world
+everyone
 goodbye
)";

  auto result = parse_unified_diff(kDiff);
  ASSERT_EQ(result.files.size(), 1u);
  ASSERT_FALSE(result.error_line.has_value()) << result.error_message;

  const auto& file = result.files[0];
  EXPECT_EQ(file.old_path, "a/hello.txt");
  EXPECT_EQ(file.new_path, "b/hello.txt");
  EXPECT_FALSE(file.is_binary);

  ASSERT_EQ(file.hunks.size(), 1u);
  const auto& hunk = file.hunks[0];
  EXPECT_EQ(hunk.header, "@@ -1,3 +1,3 @@");
  EXPECT_EQ(hunk.old_start, 1);
  EXPECT_EQ(hunk.old_count, 3);
  EXPECT_EQ(hunk.new_start, 1);
  EXPECT_EQ(hunk.new_count, 3);

  ASSERT_EQ(hunk.lines.size(), 4u);

  // Context line "hello"
  EXPECT_EQ(hunk.lines[0].type, DiffLineType::kContext);
  EXPECT_EQ(hunk.lines[0].old_line, 1);
  EXPECT_EQ(hunk.lines[0].new_line, 1);
  EXPECT_EQ(hunk.lines[0].content, "hello");

  // Deletion "-world"
  EXPECT_EQ(hunk.lines[1].type, DiffLineType::kDeletion);
  EXPECT_EQ(hunk.lines[1].old_line, 2);
  EXPECT_FALSE(hunk.lines[1].new_line.has_value());
  EXPECT_EQ(hunk.lines[1].content, "world");

  // Addition "+everyone"
  EXPECT_EQ(hunk.lines[2].type, DiffLineType::kAddition);
  EXPECT_FALSE(hunk.lines[2].old_line.has_value());
  EXPECT_EQ(hunk.lines[2].new_line, 2);
  EXPECT_EQ(hunk.lines[2].content, "everyone");

  // Context line "goodbye" — old:3, new:3
  EXPECT_EQ(hunk.lines[3].type, DiffLineType::kContext);
  EXPECT_EQ(hunk.lines[3].old_line, 3);
  EXPECT_EQ(hunk.lines[3].new_line, 3);
  EXPECT_EQ(hunk.lines[3].content, "goodbye");
}

TEST(ParseUnifiedDiff, MultipleFiles) {
  constexpr std::string_view kDiff = R"(--- a/file1.txt
+++ b/file1.txt
@@ -1 +1 @@
-old
+new
--- a/file2.txt
+++ b/file2.txt
@@ -1 +1,2 @@
 only
+added
)";

  auto result = parse_unified_diff(kDiff);
  ASSERT_EQ(result.files.size(), 2u);
  ASSERT_FALSE(result.error_line.has_value()) << result.error_message;

  // First file
  EXPECT_EQ(result.files[0].old_path, "a/file1.txt");
  EXPECT_EQ(result.files[0].new_path, "b/file1.txt");
  ASSERT_EQ(result.files[0].hunks.size(), 1u);
  ASSERT_EQ(result.files[0].hunks[0].lines.size(), 2u);

  // Second file
  EXPECT_EQ(result.files[1].old_path, "a/file2.txt");
  EXPECT_EQ(result.files[1].new_path, "b/file2.txt");
  ASSERT_EQ(result.files[1].hunks.size(), 1u);
  ASSERT_EQ(result.files[1].hunks[0].lines.size(), 2u);
}

// =========================================================================
// Edge cases
// =========================================================================

TEST(ParseUnifiedDiff, BinaryFileNotice) {
  constexpr std::string_view kDiff = R"(--- a/image.png
+++ b/image.png
Binary files a/image.png and b/image.png differ
)";

  auto result = parse_unified_diff(kDiff);
  ASSERT_EQ(result.files.size(), 1u);
  ASSERT_FALSE(result.error_line.has_value()) << result.error_message;

  EXPECT_TRUE(result.files[0].is_binary);
  EXPECT_TRUE(result.files[0].hunks.empty());
}

TEST(ParseUnifiedDiff, EmptyFileCreation) {
  constexpr std::string_view kDiff = R"(--- /dev/null
+++ b/new.txt
@@ -0,0 +1 @@
+content
)";

  auto result = parse_unified_diff(kDiff);
  ASSERT_EQ(result.files.size(), 1u);
  ASSERT_FALSE(result.error_line.has_value()) << result.error_message;

  EXPECT_EQ(result.files[0].old_path, "/dev/null");
  EXPECT_EQ(result.files[0].new_path, "b/new.txt");
  ASSERT_EQ(result.files[0].hunks.size(), 1u);
  ASSERT_EQ(result.files[0].hunks[0].lines.size(), 1u);
  EXPECT_EQ(result.files[0].hunks[0].lines[0].type, DiffLineType::kAddition);
}

TEST(ParseUnifiedDiff, FileDeletion) {
  constexpr std::string_view kDiff = R"(--- a/old.txt
+++ /dev/null
@@ -1,2 +0,0 @@
-line1
-line2
)";

  auto result = parse_unified_diff(kDiff);
  ASSERT_EQ(result.files.size(), 1u);
  ASSERT_FALSE(result.error_line.has_value()) << result.error_message;

  EXPECT_EQ(result.files[0].old_path, "a/old.txt");
  EXPECT_EQ(result.files[0].new_path, "/dev/null");
  ASSERT_EQ(result.files[0].hunks.size(), 1u);
  ASSERT_EQ(result.files[0].hunks[0].lines.size(), 2u);
  EXPECT_EQ(result.files[0].hunks[0].lines[0].type, DiffLineType::kDeletion);
  EXPECT_EQ(result.files[0].hunks[0].lines[1].type, DiffLineType::kDeletion);
}

TEST(ParseUnifiedDiff, HunkWithNoContentChange) {
  constexpr std::string_view kDiff = R"(--- a/f.txt
+++ b/f.txt
@@ -0,0 +0,0 @@
)";

  auto result = parse_unified_diff(kDiff);
  ASSERT_EQ(result.files.size(), 1u);
  ASSERT_EQ(result.files[0].hunks.size(), 1u);
  EXPECT_TRUE(result.files[0].hunks[0].lines.empty());
}

// =========================================================================
// Hunk header parsing variants
// =========================================================================

TEST(ParseUnifiedDiff, HunkHeaderWithSectionHeading) {
  constexpr std::string_view kDiff = R"(--- a/example.cpp
+++ b/example.cpp
@@ -10,7 +10,7 @@ int main() {
-context
+changed
)";

  auto result = parse_unified_diff(kDiff);
  ASSERT_EQ(result.files.size(), 1u);
  ASSERT_FALSE(result.error_line.has_value()) << result.error_message;

  const auto& hunk = result.files[0].hunks[0];
  EXPECT_EQ(hunk.old_start, 10);
  EXPECT_EQ(hunk.old_count, 7);
  EXPECT_EQ(hunk.new_start, 10);
  EXPECT_EQ(hunk.new_count, 7);
  EXPECT_TRUE(hunk.header.find("int main()") != std::string::npos);
}

TEST(ParseUnifiedDiff, HunkHeaderWithoutCount) {
  // When count is omitted, it defaults to 1.
  constexpr std::string_view kDiff = R"(--- a/f.txt
+++ b/f.txt
@@ -5 +5 @@
-context
+changed
)";

  auto result = parse_unified_diff(kDiff);
  ASSERT_EQ(result.files.size(), 1u);
  ASSERT_FALSE(result.error_line.has_value()) << result.error_message;

  const auto& hunk = result.files[0].hunks[0];
  EXPECT_EQ(hunk.old_start, 5);
  EXPECT_EQ(hunk.old_count, 1);
  EXPECT_EQ(hunk.new_start, 5);
  EXPECT_EQ(hunk.new_count, 1);
}

// =========================================================================
// "No newline at end of file" markers
// =========================================================================

TEST(ParseUnifiedDiff, NoNewlineAtEndOfFile) {
  constexpr std::string_view kDiff = R"(--- a/f.txt
+++ b/f.txt
@@ -1,3 +1,3 @@
 line1
-line2
+line2
\ No newline at end of file
 line3
)";

  auto result = parse_unified_diff(kDiff);
  ASSERT_EQ(result.files.size(), 1u);
  ASSERT_FALSE(result.error_line.has_value()) << result.error_message;

  const auto& hunk = result.files[0].hunks[0];
  ASSERT_EQ(hunk.lines.size(), 4u);
  EXPECT_EQ(hunk.lines[0].content, "line1");
  EXPECT_EQ(hunk.lines[1].content, "line2");
  EXPECT_EQ(hunk.lines[1].type, DiffLineType::kDeletion);
  EXPECT_EQ(hunk.lines[2].content, "line2");
  EXPECT_EQ(hunk.lines[2].type, DiffLineType::kAddition);
  EXPECT_EQ(hunk.lines[3].content, "line3");
  EXPECT_EQ(hunk.lines[3].type, DiffLineType::kContext);
}

// =========================================================================
// Malformed input
// =========================================================================

TEST(ParseUnifiedDiff, MalformedHunkHeader) {
  constexpr std::string_view kDiff = R"(--- a/f.txt
+++ b/f.txt
@@ -invalid @@
 context
)";

  auto result = parse_unified_diff(kDiff);
  ASSERT_EQ(result.files.size(), 1u);
  ASSERT_TRUE(result.error_line.has_value());
  EXPECT_EQ(result.error_line, 3u);
  EXPECT_FALSE(result.error_message.empty());
  ASSERT_EQ(result.files[0].hunks.size(), 1u);
  EXPECT_EQ(result.files[0].hunks[0].old_start, 0);
}

TEST(ParseUnifiedDiff, GarbageInput) {
  constexpr std::string_view kDiff = "this is not a diff\nneither is this\n";

  auto result = parse_unified_diff(kDiff);
  EXPECT_TRUE(result.files.empty());
  EXPECT_FALSE(result.error_line.has_value());
}

TEST(ParseUnifiedDiff, OnlyHeadersNoHunks) {
  constexpr std::string_view kDiff = "--- a/f.txt\n+++ b/f.txt\n";

  auto result = parse_unified_diff(kDiff);
  ASSERT_EQ(result.files.size(), 1u);
  EXPECT_TRUE(result.files[0].hunks.empty());
}

TEST(ParseUnifiedDiff, HunkWithUnknownLine) {
  constexpr std::string_view kDiff = R"(--- a/f.txt
+++ b/f.txt
@@ -1,2 +1,2 @@
 context
?unknown
)";

  auto result = parse_unified_diff(kDiff);
  ASSERT_EQ(result.files.size(), 1u);
  ASSERT_TRUE(result.error_line.has_value());
  EXPECT_EQ(result.error_line, 5u);
  ASSERT_GE(result.files[0].hunks[0].lines.size(), 2u);
}

// =========================================================================
// Hunk header malformed parsing
// =========================================================================

TEST(ParseUnifiedDiff, HunkHeaderGarbageAfterRange) {
  // "x" after old_start should be rejected
  constexpr std::string_view kDiff = R"(--- a/f.txt
+++ b/f.txt
@@ -1x,2 +1,2 @@
 context
)";

  auto result = parse_unified_diff(kDiff);
  ASSERT_EQ(result.files.size(), 1u);
  ASSERT_TRUE(result.error_line.has_value());
  EXPECT_EQ(result.error_line, 3u);
}

TEST(ParseUnifiedDiff, HunkHeaderMissingClosingAts) {
  // Missing closing @@ should be rejected
  constexpr std::string_view kDiff = R"(--- a/f.txt
+++ b/f.txt
@@ -1,2 +1,2
 context
)";

  auto result = parse_unified_diff(kDiff);
  ASSERT_EQ(result.files.size(), 1u);
  ASSERT_TRUE(result.error_line.has_value());
  EXPECT_EQ(result.error_line, 3u);
}

TEST(ParseUnifiedDiff, HunkHeaderGarbageInNewRange) {
  // "x" at end of new range should be rejected
  constexpr std::string_view kDiff = R"(--- a/f.txt
+++ b/f.txt
@@ -1 +1x @@
 context
)";

  auto result = parse_unified_diff(kDiff);
  ASSERT_EQ(result.files.size(), 1u);
  ASSERT_TRUE(result.error_line.has_value());
  EXPECT_EQ(result.error_line, 3u);
}

// =========================================================================
// Path stripping
// =========================================================================

TEST(ParseUnifiedDiff, PathWithTimestampIsStripped) {
  // POSIX unified diff format: path<TAB>timestamp
  constexpr std::string_view kDiff =
      "--- a/f.txt\t2024-01-01 12:00:00.000000000 +0000\n"
      "+++ b/f.txt\t2024-01-01 13:00:00.000000000 +0000\n"
      "@@ -1 +1 @@\n"
      " old\n";

  auto result = parse_unified_diff(kDiff);
  ASSERT_EQ(result.files.size(), 1u);
  EXPECT_EQ(result.files[0].old_path, "a/f.txt");
  EXPECT_EQ(result.files[0].new_path, "b/f.txt");
}

// =========================================================================
// Line number tracking
// =========================================================================

TEST(ParseUnifiedDiff, LineNumberTrackingWithMultipleChanges) {
  constexpr std::string_view kDiff = R"(--- a/code.cpp
+++ b/code.cpp
@@ -1,6 +1,7 @@
 keep
+added
-removed
 stay
+also_added
 still
 end
)";

  auto result = parse_unified_diff(kDiff);
  ASSERT_EQ(result.files.size(), 1u);
  const auto& lines = result.files[0].hunks[0].lines;

  EXPECT_EQ(lines[0].type, DiffLineType::kContext);
  EXPECT_EQ(lines[0].old_line, 1);
  EXPECT_EQ(lines[0].new_line, 1);

  EXPECT_EQ(lines[1].type, DiffLineType::kAddition);
  EXPECT_FALSE(lines[1].old_line.has_value());
  EXPECT_EQ(lines[1].new_line, 2);

  EXPECT_EQ(lines[2].type, DiffLineType::kDeletion);
  EXPECT_EQ(lines[2].old_line, 2);
  EXPECT_FALSE(lines[2].new_line.has_value());

  EXPECT_EQ(lines[3].type, DiffLineType::kContext);
  EXPECT_EQ(lines[3].old_line, 3);
  EXPECT_EQ(lines[3].new_line, 3);

  EXPECT_EQ(lines[4].type, DiffLineType::kAddition);
  EXPECT_FALSE(lines[4].old_line.has_value());
  EXPECT_EQ(lines[4].new_line, 4);

  EXPECT_EQ(lines[5].type, DiffLineType::kContext);
  EXPECT_EQ(lines[5].old_line, 4);
  EXPECT_EQ(lines[5].new_line, 5);

  EXPECT_EQ(lines[6].type, DiffLineType::kContext);
  EXPECT_EQ(lines[6].old_line, 5);
  EXPECT_EQ(lines[6].new_line, 6);
}

// =========================================================================
// Real-world git diff
// =========================================================================

TEST(ParseUnifiedDiff, RealWorldGitDiff) {
  constexpr std::string_view kDiff = R"(diff --git a/src/main.cpp b/src/main.cpp
index 123abc..456def 100644
--- a/src/main.cpp
+++ b/src/main.cpp
@@ -1,5 +1,7 @@
 #include <iostream>
+#include <string>
 
 int main() {
-  std::cout << "Hello" << std::endl;
+  std::string name = "World";
+  std::cout << "Hello, " << name << std::endl;
   return 0;
 }
)";

  auto result = parse_unified_diff(kDiff);
  ASSERT_EQ(result.files.size(), 1u);
  EXPECT_EQ(result.files[0].old_path, "a/src/main.cpp");
  EXPECT_EQ(result.files[0].new_path, "b/src/main.cpp");
  ASSERT_EQ(result.files[0].hunks.size(), 1u);
  const auto& hunk = result.files[0].hunks[0];
  ASSERT_EQ(hunk.lines.size(), 9u);
  EXPECT_EQ(hunk.lines[0].content, "#include <iostream>");
  EXPECT_EQ(hunk.lines[1].type, DiffLineType::kAddition);
  EXPECT_EQ(hunk.lines[1].content, "#include <string>");
  EXPECT_EQ(hunk.lines[2].content, "");
  EXPECT_EQ(hunk.lines[2].type, DiffLineType::kContext);
  EXPECT_EQ(hunk.lines[3].content, "int main() {");
  EXPECT_EQ(hunk.lines[4].type, DiffLineType::kDeletion);
  EXPECT_EQ(hunk.lines[4].content, R"(  std::cout << "Hello" << std::endl;)");
  EXPECT_EQ(hunk.lines[5].type, DiffLineType::kAddition);
  EXPECT_EQ(hunk.lines[5].content, R"(  std::string name = "World";)");
  EXPECT_EQ(hunk.lines[6].type, DiffLineType::kAddition);
  EXPECT_EQ(hunk.lines[6].content, R"(  std::cout << "Hello, " << name << std::endl;)");
  EXPECT_EQ(hunk.lines[7].content, "  return 0;");
  EXPECT_EQ(hunk.lines[7].type, DiffLineType::kContext);
  EXPECT_EQ(hunk.lines[8].content, "}");
  EXPECT_EQ(hunk.lines[8].type, DiffLineType::kContext);
}

// =========================================================================
// Windows-style line endings (\r\n)
// =========================================================================

TEST(ParseUnifiedDiff, WindowsLineEndings) {
  constexpr std::string_view kDiff =
      "--- a/win.txt\r\n+++ b/win.txt\r\n@@ -1,2 +1,3 @@\r\n keep\r\n+added\r\n";

  auto result = parse_unified_diff(kDiff);
  ASSERT_EQ(result.files.size(), 1u);
  ASSERT_FALSE(result.error_line.has_value()) << result.error_message;
  ASSERT_EQ(result.files[0].hunks.size(), 1u);
  ASSERT_EQ(result.files[0].hunks[0].lines.size(), 2u);
  EXPECT_EQ(result.files[0].hunks[0].lines[0].content, "keep");
  EXPECT_EQ(result.files[0].hunks[0].lines[1].type, DiffLineType::kAddition);
  EXPECT_EQ(result.files[0].hunks[0].lines[1].content, "added");
}

// =========================================================================
// Large diff with many lines
// =========================================================================

TEST(ParseUnifiedDiff, LargeDiff) {
  std::string diff = "--- a/large.txt\n+++ b/large.txt\n@@ -1,100 +1,100 @@\n";
  for (int i = 1; i <= 100; ++i) {
    diff += " context_line_" + std::to_string(i) + "\n";
  }

  auto result = parse_unified_diff(diff);
  ASSERT_EQ(result.files.size(), 1u);
  ASSERT_EQ(result.files[0].hunks.size(), 1u);
  EXPECT_EQ(result.files[0].hunks[0].lines.size(), 100u);
  EXPECT_EQ(result.files[0].hunks[0].lines[0].content, "context_line_1");
  EXPECT_EQ(result.files[0].hunks[0].lines[99].content, "context_line_100");
}

}  // namespace
}  // namespace terminal_ui_kit
