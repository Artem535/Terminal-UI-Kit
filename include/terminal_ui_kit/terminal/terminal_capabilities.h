#pragma once

#include <string>

namespace terminal_ui_kit {
namespace terminal {

// The color capability of a terminal, ordered from least to most expressive.
enum class ColorDepth {
  // No color at all (monochrome; e.g. TERM=dumb, NO_COLOR, unknown terminal).
  kNone,
  // The base 16 ANSI colors.
  k16Color,
  // The 256-color palette (e.g. xterm-256color, tmux-256color).
  k256Color,
  // 24-bit truecolor (COLORTERM=truecolor).
  kTrueColor,
};

// A snapshot of what the current terminal is believed to support (PRD section
// 63, "terminal integrations"). Produced by TerminalDetector::Detect from
// environment signals plus explicit overrides; never probed live.
//
// The struct is intentionally plain data: it is copied by value and retained
// freely. All defaults are the most conservative (lowest-capability) state;
// detection only ever raises them, except that NO_COLOR and explicit
// overrides may lower the color depth / a capability.
struct TerminalCapabilities {
  ColorDepth color_depth = ColorDepth::kNone;
  // Can display Unicode (non-ASCII glyphs) rather than only 7-bit US-ASCII.
  bool unicode = false;
  // Supports mouse input reporting (SGR / UTF-8 mouse escapes).
  bool mouse = false;
  // Supports bracketed paste mode (CSI 200~ ... CSI 201~).
  bool bracketed_paste = false;
  // Supports hyperlinks (OSC 8).
  bool hyperlinks = false;
  // Supports terminal graphics via the Kitty graphics protocol (APC).
  bool kitty_graphics = false;
  // Supports sixel graphics (DCS P ... ST).
  bool sixel = false;
  // Supports inline images via the iTerm2 escape protocol (OSC 1337).
  bool iterm_images = false;
  // Supports OSC 52 clipboard control (set the OS clipboard from the terminal).
  bool osc52 = false;
  // Supports the alternate screen buffer (CSI ? 1049 h/l).
  bool alternate_screen = false;
  // Running inside tmux.
  bool tmux = false;
  // Running inside GNU screen.
  bool screen = false;
  // Running over an SSH session.
  bool ssh = false;
  // A stable, human-readable name for the detected terminal/program. Empty
  // when nothing recognizable was found.
  std::string terminal_identity;
};

}  // namespace terminal
}  // namespace terminal_ui_kit