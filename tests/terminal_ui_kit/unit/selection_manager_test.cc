#include "terminal_ui_kit/core/selection.h"

#include <gtest/gtest.h>

#include "terminal_ui_kit/document/streaming_document.h"

namespace terminal_ui_kit {
namespace {

TEST(SelectionManager, StartsWithNoSelection) {
  SelectionManager sm;
  EXPECT_FALSE(sm.has_selection());
  EXPECT_FALSE(sm.is_selecting());
  EXPECT_TRUE(sm.range().is_empty());
}

TEST(SelectionManager, StartSetsAnchor) {
  SelectionManager sm;
  sm.start(TextPosition{2, 5});
  EXPECT_TRUE(sm.is_selecting());
  EXPECT_FALSE(sm.has_selection());  // anchor == active
  EXPECT_TRUE(sm.range().is_empty());
}

TEST(SelectionManager, ExtendToSetsRange) {
  SelectionManager sm;
  sm.start(TextPosition{1, 0});
  sm.extend_to(TextPosition{3, 5});
  EXPECT_TRUE(sm.has_selection());
  EXPECT_EQ(sm.range().start, TextPosition(1, 0));
  EXPECT_EQ(sm.range().end, TextPosition(3, 5));
}

TEST(SelectionManager, ClearResetsState) {
  SelectionManager sm;
  sm.start(TextPosition{1, 0});
  sm.extend_to(TextPosition{3, 0});
  EXPECT_TRUE(sm.has_selection());
  sm.clear();
  EXPECT_FALSE(sm.has_selection());
  EXPECT_FALSE(sm.is_selecting());
  EXPECT_TRUE(sm.range().is_empty());
}

TEST(SelectionManager, SingleLineSelectedText) {
  StreamingDocument doc;
  doc.append("hello world");
  doc.finish();

  SelectionManager sm;
  sm.start(TextPosition{0, 0});
  sm.extend_to(TextPosition{0, 5});
  EXPECT_EQ(sm.selected_text(doc), "hello");
}

TEST(SelectionManager, MultiLineSelectedText) {
  StreamingDocument doc;
  doc.append("abc\ndef\nghi\njkl");
  doc.finish();

  SelectionManager sm;
  sm.start(TextPosition{0, 1});
  sm.extend_to(TextPosition{2, 2});
  EXPECT_EQ(sm.selected_text(doc), "bc\ndef\ngh");
}

TEST(SelectionManager, RangeIsNormalized) {
  SelectionManager sm;
  // Active before anchor -> range should still be [active, anchor)
  sm.start(TextPosition{5, 10});
  sm.extend_to(TextPosition{2, 3});
  auto r = sm.range();
  EXPECT_EQ(r.start, TextPosition(2, 3));
  EXPECT_EQ(r.end, TextPosition(5, 10));
}

}  // namespace
}  // namespace terminal_ui_kit
