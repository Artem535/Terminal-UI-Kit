#include "terminal_ui_kit/components/code_view.h"

#include <cstddef>
#include <string>

#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

// FTXUI's Screen::ToString() interleaves ANSI SGR escape sequences (e.g. the
// GrayDark gutter colour) with the visible glyphs. This strips CSI sequences
// so assertions can be written against the plain text.
std::string StripAnsi(const std::string& in) {
  std::string out;
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size();) {
    if (in[i] == '\x1b' && i + 1 < in.size() && in[i + 1] == '[') {
      std::size_t j = i + 2;
      while (j < in.size() && !(static_cast<unsigned char>(in[j]) >= 0x40 &&
                                static_cast<unsigned char>(in[j]) < 0x80))
        ++j;
      if (j < in.size()) ++j;  // skip the final byte of the sequence
      i = j;
    } else if (in[i] != '\r') {  // discard FTXUI's carriage-return line suffix
      out.push_back(in[i++]);
    } else {
      ++i;
    }
  }
  return out;
}

// Builds a document with `count` lines whose content is the line number
// itself ("1\n2\n3\n..."), so the rendered line-number gutter shows values
// covering short, exact-width and beyond-width cases in one document.
std::string NumberedDocument(std::size_t count) {
  std::string doc;
  for (std::size_t i = 1; i <= count; ++i) {
    doc += std::to_string(i);
    if (i != count) doc += "\n";
  }
  return doc;
}

TEST(CodeView, ShortNumberIsRightAlignedWithinGutter) {
  CodeViewOptions opts;
  opts.show_line_numbers = true;
  opts.gutter_width = 4;
  // Line numbers are 1-based; with a single line the gutter shows "   1".
  std::string text = StripAnsi(test_support::render_to_text(CodeView("hello\n", opts), 30, 3));
  EXPECT_NE(text.find("   1 hello"), std::string::npos);
}

TEST(CodeView, NumberLongerThanGutterRendersFullyWithoutHugePadding) {
  CodeViewOptions opts;
  opts.show_line_numbers = true;
  opts.gutter_width = 4;
  // 10000 lines -> last line number is 10000 (5 digits > 4-wide gutter).
  std::string text =
      StripAnsi(test_support::render_to_text(CodeView(NumberedDocument(10000), opts), 40, 10010));
  // The 5-digit number must be present and untruncated.
  EXPECT_NE(text.find("10000"), std::string::npos);
  // No enormous padding run may be allocated/rendered (old bug produced a
  // huge string of spaces here).
  EXPECT_EQ(text.find(std::string(50, ' ')), std::string::npos);
}

TEST(CodeView, GutterWidthZeroIsSafe) {
  CodeViewOptions opts;
  opts.show_line_numbers = true;
  opts.gutter_width = 0;
  std::string text = StripAnsi(test_support::render_to_text(CodeView("abc\n", opts), 30, 3));
  // No gutter padding; number renders unpadded, immediately followed by the
  // separator space then the code.
  EXPECT_NE(text.find("1 abc"), std::string::npos);
}

TEST(CodeView, GutterWidthLargerThanDigits) {
  CodeViewOptions opts;
  opts.show_line_numbers = true;
  opts.gutter_width = 6;
  std::string text = StripAnsi(test_support::render_to_text(CodeView("x\n", opts), 30, 3));
  EXPECT_NE(text.find("     1 x"), std::string::npos);
}

TEST(CodeView, GutterWidthEqualToDigits) {
  CodeViewOptions opts;
  opts.show_line_numbers = true;
  opts.gutter_width = 1;
  std::string text = StripAnsi(test_support::render_to_text(CodeView("x\n", opts), 30, 3));
  // Exactly width -> no padding.
  EXPECT_NE(text.find("1 x"), std::string::npos);
}

}  // namespace
}  // namespace terminal_ui_kit