#include "terminal_ui_kit/diff/diff_model.h"

#include <gtest/gtest.h>

namespace terminal_ui_kit::diff {
namespace {

TEST(DiffModel, DefaultLineIsContextWithNoNumbers) {
  DiffLine line;
  EXPECT_EQ(line.type, DiffLineType::kContext);
  EXPECT_FALSE(line.old_line.has_value());
  EXPECT_FALSE(line.new_line.has_value());
  EXPECT_TRUE(line.content.spans().empty());
}

TEST(DiffModel, DefaultHunkHasEmptyHeaderAndNoLines) {
  DiffHunk hunk;
  EXPECT_TRUE(hunk.header.empty());
  EXPECT_TRUE(hunk.lines.empty());
}

TEST(DiffModel, DefaultFileHasEmptyPathsAndNoHunks) {
  DiffFile file;
  EXPECT_TRUE(file.old_path.empty());
  EXPECT_TRUE(file.new_path.empty());
  EXPECT_FALSE(file.is_binary);
  EXPECT_TRUE(file.hunks.empty());
}

TEST(DiffModel, DefaultDocumentHasNoFiles) {
  DiffDocument document;
  EXPECT_TRUE(document.files.empty());
}

TEST(DiffModel, LineCarriesOldAndNewNumbers) {
  DiffLine line;
  line.type = DiffLineType::kAddition;
  line.old_line = std::nullopt;
  line.new_line = 42;
  EXPECT_EQ(line.type, DiffLineType::kAddition);
  EXPECT_FALSE(line.old_line.has_value());
  ASSERT_TRUE(line.new_line.has_value());
  EXPECT_EQ(*line.new_line, 42);
}

TEST(DiffModel, LineContentIsStyledText) {
  DiffLine line;
  line.content.append(TextSpan{"hello", TextStyle{}, std::nullopt});
  ASSERT_EQ(line.content.spans().size(), 1u);
  EXPECT_EQ(line.content.spans()[0].text, "hello");
}

TEST(DiffModel, FileCanBeMarkedBinary) {
  DiffFile file;
  file.is_binary = true;
  EXPECT_TRUE(file.is_binary);
}

}  // namespace
}  // namespace terminal_ui_kit::diff
