#include "terminal_ui_kit/syntax/syntax_theme.h"

namespace terminal_ui_kit {
namespace {

constexpr Color rgb(unsigned int value) {
  return Color{
      static_cast<unsigned char>((value >> 16) & 0xff),
      static_cast<unsigned char>((value >> 8) & 0xff),
      static_cast<unsigned char>(value & 0xff),
  };
}

TextStyle foreground(unsigned int value, bool bold = false, bool dim = false) {
  TextStyle style;
  style.bold = bold;
  style.dim = dim;
  style.foreground = rgb(value);
  return style;
}

}  // namespace

SyntaxTheme default_dark_syntax_theme(const Theme& theme) {
  SyntaxTheme syntax;
  syntax.keyword = foreground(0xff7b72, true);
  syntax.type = foreground(0x79c0ff);
  syntax.function = foreground(0xd2a8ff);
  syntax.variable = theme.primary;
  syntax.variable.background.reset();
  syntax.string = foreground(0xa5d6ff);
  syntax.number = foreground(0xf2cc60);
  syntax.comment = foreground(0x8b949e, false, true);
  syntax.operator_style = foreground(0xf0f6fc);
  syntax.property = foreground(0x7ee787);
  syntax.namespace_style = foreground(0x56d4dd);
  syntax.macro = foreground(0xffa657, true);
  syntax.constant = foreground(0xffdf5d);
  return syntax;
}

SyntaxTheme default_light_syntax_theme(const Theme& theme) {
  SyntaxTheme syntax;
  syntax.keyword = foreground(0xcf222e, true);
  syntax.type = foreground(0x0550ae);
  syntax.function = foreground(0x8250df);
  syntax.variable = theme.primary;
  syntax.variable.background.reset();
  syntax.string = foreground(0x0a3069);
  syntax.number = foreground(0x953800);
  syntax.comment = foreground(0x6e7781, false, true);
  syntax.operator_style = foreground(0x24292f);
  syntax.property = foreground(0x116329);
  syntax.namespace_style = foreground(0x0969da);
  syntax.macro = foreground(0xbc4a00, true);
  syntax.constant = foreground(0x6639ba);
  return syntax;
}

}  // namespace terminal_ui_kit
