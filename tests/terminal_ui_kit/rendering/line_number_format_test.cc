#include "terminal_ui_kit/components/line_number_format.h"

#include <string>

#include "terminal_ui_kit/components/code_view.h"
#include "terminal_ui_kit/components/virtual_document.h"
#include "terminal_ui_kit/document/streaming_document.h"
#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

// ---- format_line_number (shared gutter helper) ---------------------------

TEST(LineNumberFormat, ShortNumbersAreRightAligned) {
  EXPECT_EQ(format_line_number(1, 4), "   1");
  EXPECT_EQ(format_line_number(9, 4), "   9");
  EXPECT_EQ(format_line_number(99, 4), "  99");
}

TEST(LineNumberFormat, NumberEqualToWidthHasNoPadding) {
  EXPECT_EQ(format_line_number(9999, 4), "9999");
}

TEST(LineNumberFormat, NumberWiderThanWidthRendersInFull) {
  // Regression: the old inline code did `std::string(width - num.size(), ' ')`
  // where `width` is an int and `num.size()` is 64-bit unsigned, so a number
  // wider than the gutter underflowed to a huge std::size_t and produced an
  // enormous padding string.
  EXPECT_EQ(format_line_number(10000, 4), "10000");
  EXPECT_EQ(format_line_number(99999, 4), "99999");
  EXPECT_EQ(format_line_number(100000, 4), "100000");
}

TEST(LineNumberFormat, LargeLineNumberRendersInFull) {
  EXPECT_EQ(format_line_number(1000000, 4), "1000000");
  EXPECT_EQ(format_line_number(1000000000000ULL, 5), "1000000000000");
}

TEST(LineNumberFormat, ZeroWidthIsSafe) {
  EXPECT_EQ(format_line_number(123, 0), "123");
  EXPECT_EQ(format_line_number(7, 0), "7");
}

TEST(LineNumberFormat, WidthSmallerThanDigitCount) {
  EXPECT_EQ(format_line_number(100, 2), "100");
  EXPECT_EQ(format_line_number(12345, 1), "12345");
}

TEST(LineNumberFormat, WidthEqualToDigitCount) {
  EXPECT_EQ(format_line_number(100, 3), "100");
  EXPECT_EQ(format_line_number(9, 1), "9");
}

TEST(LineNumberFormat, WidthLargerThanDigitCount) {
  EXPECT_EQ(format_line_number(100, 5), "  100");
  EXPECT_EQ(format_line_number(1, 8), "       1");
}

// ---- VirtualDocument rendering -------------------------------------------

TEST(LineNumberFormat, VirtualDocumentWideNumberRendersInFull) {
  // 100000 lines; the bottom line number has 6 digits, wider than the default
  // gutter width of 5. The old implementation underflowed on this exact case.
  StreamingDocument doc;
  for (std::size_t i = 1; i <= 100000; ++i) {
    doc.append("line" + std::to_string(i) + "\n");
  }
  doc.finish();

  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.show_line_numbers = true;
  opts.follow = true;          // scroll to the bottom, where the wide numbers are.
  VirtualDocument view(opts);  // line_number_width defaults to 5.

  std::string text = test_support::render_to_text(view.component()->Render(), 40, 8);
  // No enormous padding is produced after the (previously underflowing)
  // subtraction: the whole rendered output stays small.
  EXPECT_LT(text.size(), 2000u);
  // The 6-digit line number is rendered fully (never truncated).
  EXPECT_NE(text.find("10000"), std::string::npos);
}

TEST(LineNumberFormat, VirtualDocumentZeroWidthIsSafe) {
  StreamingDocument doc;
  doc.append("hello\nworld");
  doc.finish();

  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.show_line_numbers = true;
  opts.line_number_width = 0;
  opts.follow = false;
  VirtualDocument view(opts);

  std::string text = test_support::render_to_text(view.component()->Render(), 20, 3);
  EXPECT_NE(text.find("hello"), std::string::npos);
  EXPECT_NE(text.find("world"), std::string::npos);
  EXPECT_LT(text.size(), 1000u);
}

// ---- CodeView rendering ---------------------------------------------------

TEST(LineNumberFormat, CodeViewKeepsShortNumberAlignment) {
  CodeViewOptions opts;
  opts.show_line_numbers = true;
  opts.line_number_width = 4;

  std::string text = test_support::render_to_text(CodeView("alpha", opts), 20, 2);
  EXPECT_NE(text.find("   1"), std::string::npos);
  EXPECT_NE(text.find("alpha"), std::string::npos);
}

TEST(LineNumberFormat, CodeViewZeroWidthIsSafe) {
  CodeViewOptions opts;
  opts.show_line_numbers = true;
  opts.line_number_width = 0;

  std::string text = test_support::render_to_text(CodeView("hello\nworld", opts), 20, 2);
  // The bare number and the (single collapsed) content line both render.
  EXPECT_NE(text.find("1"), std::string::npos);
  EXPECT_NE(text.find("helloworld"), std::string::npos);
  EXPECT_LT(text.size(), 1000u);
}

}  // namespace
}  // namespace terminal_ui_kit