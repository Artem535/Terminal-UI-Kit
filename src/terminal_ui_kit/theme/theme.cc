#include "terminal_ui_kit/theme/theme.h"

#include <cstdint>

namespace terminal_ui_kit {
namespace {

constexpr Color hex(std::uint32_t rgb) {
  return Color{
      static_cast<std::uint8_t>((rgb >> 16) & 0xFF),
      static_cast<std::uint8_t>((rgb >> 8) & 0xFF),
      static_cast<std::uint8_t>(rgb & 0xFF),
  };
}

Theme make_dark_theme() {
  Theme theme;

  theme.primary.foreground = hex(0xe6edf3);
  theme.secondary.foreground = hex(0x848d97);
  theme.muted.foreground = hex(0x6e7681);
  theme.muted.dim = true;
  theme.success.foreground = hex(0x3fb950);
  theme.warning.foreground = hex(0xd29922);
  theme.error.foreground = hex(0xf85149);
  theme.error.bold = true;
  theme.accent.foreground = hex(0x58a6ff);
  theme.code.foreground = hex(0xe6edf3);
  theme.code.background = hex(0x161b22);
  theme.addition.foreground = hex(0x3fb950);
  theme.addition.background = hex(0x0f2c1e);
  theme.deletion.foreground = hex(0xf85149);
  theme.deletion.background = hex(0x331418);
  theme.border.foreground = hex(0x30363d);
  theme.selected.background = hex(0x142f60);
  theme.focused.foreground = hex(0x58a6ff);
  theme.focused.bold = true;

  return theme;
}

Theme make_light_theme() {
  Theme theme;

  theme.primary.foreground = hex(0x1f2328);
  theme.secondary.foreground = hex(0x656d76);
  theme.muted.foreground = hex(0x8c959f);
  theme.muted.dim = true;
  theme.success.foreground = hex(0x1a7f37);
  theme.warning.foreground = hex(0x9a6700);
  theme.error.foreground = hex(0xd1242f);
  theme.error.bold = true;
  theme.accent.foreground = hex(0x0969da);
  theme.code.foreground = hex(0x1f2328);
  theme.code.background = hex(0xf6f8fa);
  theme.addition.foreground = hex(0x1a7f37);
  theme.addition.background = hex(0xe6ffec);
  theme.deletion.foreground = hex(0xd1242f);
  theme.deletion.background = hex(0xffebe9);
  theme.border.foreground = hex(0xd1d9e0);
  theme.selected.background = hex(0xddf4ff);
  theme.focused.foreground = hex(0x0969da);
  theme.focused.bold = true;

  return theme;
}

}  // namespace

const Theme& default_dark_theme() {
  static const Theme theme = make_dark_theme();
  return theme;
}

const Theme& default_light_theme() {
  static const Theme theme = make_light_theme();
  return theme;
}

}  // namespace terminal_ui_kit
