#include "terminal_ui_kit/components/code_view.h"

#include <string>

#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

// CodeView colours the gutter and code text, so the raw Screen::ToString()
// output is interleaved with ANSI escape sequences. Strip them before
// matching so tests assert on the visible glyphs only.
std::string strip_ansi(const std::string& in) {
  std::string out;
  out.reserve(in.size());
  std::size_t i = 0;
  while (i < in.size()) {
    if (in[i] == '\x1b' && i + 1 < in.size() && in[i + 1] == '[') {
      i += 2;
      while (i < in.size() && !(in[i] >= 0x40 && in[i] <= 0x7e)) {
        ++i;
      }
      if (i < in.size()) {
        ++i;  // consume the CSI final byte
      }
      continue;
    }
    out.push_back(in[i]);
    ++i;
  }
  return out;
}

std::string render_code(const CodeViewOptions& opts, const std::string& code, int width,
                        int height) {
  return strip_ansi(test_support::render_to_text(CodeView(code, opts), width, height));
}

TEST(CodeView, ShortLineNumberRightAlignedToDefaultWidth) {
  CodeViewOptions opts;
  opts.show_line_numbers = true;
  // Default gutter width is 4, so a 1-digit number is right-aligned in it.
  std::string text = render_code(opts, "hello", 12, 1);
  EXPECT_NE(text.find("   1 hello"), std::string::npos);
}

TEST(CodeView, NumberWiderThanConfiguredGutterRendersWithoutHugePadding) {
  CodeViewOptions opts;
  opts.show_line_numbers = true;
  // Gutter width 1 is narrower than the 2-digit wrapped-segment numbers.
  opts.line_number_width = 1;
  // 1000 'x' chars wrap (at width 80) into 13 segments, indexed 1..13, so the
  // last rendered segment carries a 2-digit line number wider than the gutter.
  std::string code(1000, 'x');
  std::string text = render_code(opts, code, 90, 14);
  // The 2-digit number "13" is rendered fully (not truncated), and it is
  // immediately followed by the single-space separator then the code text -
  // i.e. no enormous padding string is inserted (the old `width - digits`
  // arithmetic underflows and would emit a gigantic run of spaces in front of
  // the number).
  EXPECT_NE(text.find("13 x"), std::string::npos);
}

TEST(CodeView, ZeroGutterWidthIsSafe) {
  CodeViewOptions opts;
  opts.show_line_numbers = true;
  opts.line_number_width = 0;
  std::string text = render_code(opts, "hello", 12, 1);
  EXPECT_NE(text.find("1 hello"), std::string::npos);
}

}  // namespace
}  // namespace terminal_ui_kit
