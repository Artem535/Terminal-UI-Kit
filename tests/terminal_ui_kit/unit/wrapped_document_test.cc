#include "terminal_ui_kit/document/wrapped_document.h"

#include <string>

#include <gtest/gtest.h>

#include "terminal_ui_kit/document/streaming_document.h"

namespace terminal_ui_kit {
namespace {

TEST(WrappedDocument, EmptyDocHasNoLines) {
  StreamingDocument doc;
  WrappedDocument wrapped(40);
  wrapped.rebuild_from(doc);
  EXPECT_EQ(wrapped.display_line_count(), 0U);
}

TEST(WrappedDocument, ShortLineProducesOneDisplayLine) {
  StreamingDocument doc;
  doc.append("hello");
  doc.finish();

  WrappedDocument wrapped(40);
  wrapped.rebuild_from(doc);
  ASSERT_EQ(wrapped.display_line_count(), 1U);
  EXPECT_EQ(wrapped.display_line_at(0).text, "hello");
  EXPECT_EQ(wrapped.display_line_at(0).logical_line, 0U);
  EXPECT_EQ(wrapped.display_line_at(0).sub_line, 0U);
}

TEST(WrappedDocument, LongLineWrapsIntoMultipleDisplayLines) {
  StreamingDocument doc;
  doc.append(std::string(100, 'X'));
  doc.finish();

  WrappedDocument wrapped(30);
  wrapped.rebuild_from(doc);
  EXPECT_GT(wrapped.display_line_count(), 3U);
  for (std::size_t i = 0; i < wrapped.display_line_count(); ++i) {
    EXPECT_EQ(wrapped.display_line_at(i).logical_line, 0U);
    EXPECT_EQ(wrapped.display_line_at(i).sub_line, i);
  }
}

TEST(WrappedDocument, AppendAddsOnlyNewDisplayLines) {
  StreamingDocument doc;
  doc.append("first line");
  doc.finish();

  WrappedDocument wrapped(40);
  wrapped.rebuild_from(doc);
  ASSERT_EQ(wrapped.display_line_count(), 1U);

  doc.append("second line");
  doc.finish();
  wrapped.append_from(doc);
  ASSERT_EQ(wrapped.display_line_count(), 2U);
  EXPECT_EQ(wrapped.display_line_at(1).text, "second line");
  EXPECT_EQ(wrapped.display_line_at(1).logical_line, 1U);
}

TEST(WrappedDocument, WidthChangeRebuildsAllLines) {
  StreamingDocument doc;
  doc.append(std::string(50, 'A'));
  doc.finish();

  WrappedDocument wrapped(30);
  wrapped.rebuild_from(doc);
  const std::size_t wide_count = wrapped.display_line_count();

  wrapped.handle_width_change(60, doc);
  EXPECT_LT(wrapped.display_line_count(), wide_count);
}

TEST(WrappedDocument, ReplaceTailRewrapsLastLine) {
  StreamingDocument doc;
  doc.append("short");
  doc.finish();
  doc.append("this is a much longer replacement line");
  doc.finish();

  WrappedDocument wrapped(20);
  wrapped.rebuild_from(doc);
  const std::size_t before_replacement = wrapped.display_line_count();

  StreamingDocument doc2;
  doc2.append("short");
  doc2.finish();
  doc2.append("tiny");
  doc2.finish();

  wrapped.replace_tail(doc2);
  EXPECT_LE(wrapped.display_line_count(), before_replacement);
  EXPECT_EQ(wrapped.display_line_at(wrapped.display_line_count() - 1).text, "tiny");
}

TEST(WrappedDocument, FirstDisplayLineForSingleLine) {
  StreamingDocument doc;
  doc.append("hello");
  doc.finish();

  WrappedDocument wrapped(40);
  wrapped.rebuild_from(doc);
  EXPECT_EQ(wrapped.first_display_line_for(0), 0U);
}

TEST(WrappedDocument, FirstDisplayLineForMultiLineWrap) {
  StreamingDocument doc;
  doc.append(std::string(100, 'X'));
  doc.finish();
  doc.append("second");
  doc.finish();

  WrappedDocument wrapped(30);
  wrapped.rebuild_from(doc);
  EXPECT_EQ(wrapped.first_display_line_for(0), 0U);
  EXPECT_EQ(wrapped.display_line_at(wrapped.first_display_line_for(1)).logical_line, 1U);
}

TEST(WrappedDocument, ClearResetsAllState) {
  StreamingDocument doc;
  doc.append("hello");
  doc.finish();

  WrappedDocument wrapped(40);
  wrapped.rebuild_from(doc);
  EXPECT_GT(wrapped.display_line_count(), 0U);

  wrapped.clear();
  EXPECT_EQ(wrapped.display_line_count(), 0U);
}

TEST(WrappedDocument, AppendAfterWidthChangeWorks) {
  StreamingDocument doc;
  doc.append("first");
  doc.finish();

  WrappedDocument wrapped(40);
  wrapped.rebuild_from(doc);
  wrapped.handle_width_change(10, doc);

  doc.append("second");
  doc.finish();
  wrapped.append_from(doc);
  ASSERT_GE(wrapped.display_line_count(), 2U);
  EXPECT_EQ(wrapped.display_line_at(wrapped.display_line_count() - 1).logical_line, 1U);
}

TEST(WrappedDocument, EmptyLineProducesOneDisplayLine) {
  StreamingDocument doc;
  doc.finish();

  WrappedDocument wrapped(40);
  wrapped.rebuild_from(doc);
  ASSERT_EQ(wrapped.display_line_count(), 1U);
  EXPECT_TRUE(wrapped.display_line_at(0).text.empty());
}

TEST(WrappedDocument, OutOfRangeThrows) {
  StreamingDocument doc;
  WrappedDocument wrapped(40);
  wrapped.rebuild_from(doc);
  EXPECT_THROW(static_cast<void>(wrapped.display_line_at(0)), std::out_of_range);
}

}  // namespace
}  // namespace terminal_ui_kit