#include "terminal_ui_kit/document/streaming_document.h"

#include <string>

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TEST(StreamingDocument, StartsEmpty) {
  StreamingDocument document;
  EXPECT_EQ(document.line_count(), 0U);
  EXPECT_EQ(document.revision(), 0U);
}

TEST(StreamingDocument, SplitsLinesAcrossChunks) {
  StreamingDocument document;
  document.append("one\ntwo");
  document.append("\nthree");
  EXPECT_EQ(document.line_count(), 2U);
  EXPECT_EQ(document.line_at(0), "one");
  EXPECT_EQ(document.line_at(1), "two");
  document.finish();
  EXPECT_EQ(document.line_at(2), "three");
}

TEST(StreamingDocument, HandlesSplitUtf8) {
  StreamingDocument document;
  document.append("caf\xC3");
  EXPECT_EQ(document.line_count(), 0U);
  document.append("\xA9");
  document.finish();
  EXPECT_EQ(document.line_at(0), "café");
}

TEST(StreamingDocument, ReplacesPendingTail) {
  StreamingDocument document;
  document.append("progress");
  document.replace_tail("done");
  document.finish();
  EXPECT_EQ(document.line_at(0), "done");
}

TEST(StreamingDocument, FinishCommitsEmptyLine) {
  StreamingDocument document;
  document.finish();
  EXPECT_EQ(document.line_count(), 1U);
  EXPECT_EQ(document.line_at(0), "");
}

TEST(StreamingDocument, FinishFlushesIncompleteUtf8) {
  StreamingDocument document;
  document.append("bad\xE2");
  document.finish();
  EXPECT_EQ(document.line_at(0), "bad\xEF\xBF\xBD");
}

TEST(StreamingDocument, InvalidBytesBecomeReplacement) {
  StreamingDocument document;
  const std::string input = std::string("a\xFF") + "b\n";
  const std::string expected = std::string("a\xEF\xBF\xBD") + "b";
  document.append(input);
  EXPECT_EQ(document.line_at(0), expected);
}

TEST(StreamingDocument, StripsCarriageReturn) {
  StreamingDocument document;
  document.append("line\r\n");
  EXPECT_EQ(document.line_at(0), "line");
}

TEST(StreamingDocument, RevisionIncrementsOncePerMutation) {
  StreamingDocument document;
  document.append("x");
  EXPECT_EQ(document.revision(), 1U);
  document.replace_tail("y");
  EXPECT_EQ(document.revision(), 2U);
  document.finish();
  EXPECT_EQ(document.revision(), 3U);
  document.clear();
  EXPECT_EQ(document.revision(), 4U);
}

TEST(StreamingDocument, OutOfRangeThrows) {
  StreamingDocument document;
  EXPECT_THROW(static_cast<void>(document.line_at(0)), std::out_of_range);
}

}  // namespace
}  // namespace terminal_ui_kit
