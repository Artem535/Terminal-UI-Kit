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
  syntax.keyword = foreground(0x79c0ff, true);
  syntax.type = foreground(0x56d4dd);
  syntax.function = foreground(0xe3b341);
  syntax.variable = theme.primary;
  syntax.variable.background.reset();
  syntax.string = foreground(0x7ee787);
  syntax.number = foreground(0xffa657);
  syntax.comment = foreground(0x8b949e, false, true);
  syntax.operator_style = foreground(0xf0f6fc);
  syntax.property = foreground(0xa5d6ff);
  syntax.namespace_style = foreground(0x56d4dd);
  syntax.macro = foreground(0xff7b72, true);
  syntax.constant = foreground(0xf2cc60);
  return syntax;
}

SyntaxTheme default_light_syntax_theme(const Theme& theme) {
  SyntaxTheme syntax;
  syntax.keyword = foreground(0x0969da, true);
  syntax.type = foreground(0x0a7b83);
  syntax.function = foreground(0x9a6700);
  syntax.variable = theme.primary;
  syntax.variable.background.reset();
  syntax.string = foreground(0x116329);
  syntax.number = foreground(0xbc4a00);
  syntax.comment = foreground(0x6e7781, false, true);
  syntax.operator_style = foreground(0x24292f);
  syntax.property = foreground(0x0550ae);
  syntax.namespace_style = foreground(0x0969da);
  syntax.macro = foreground(0xcf222e, true);
  syntax.constant = foreground(0x953800);
  return syntax;
}

}  // namespace terminal_ui_kit
