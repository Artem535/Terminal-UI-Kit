#pragma once

#include <string>

namespace terminal_ui_kit {
namespace terminal {

// The color output capability of the terminal. Ordered from least to most
// capable so callers can compare with `>=`/`<=`.
enum class ColorDepth {
  // Monochrome: no ANSI color output (e.g. TERM=dumb, or NO_COLOR set).
  kNone,
  // The classic 16 ANSI colors (8 foreground + 8 "bright" foreground).
  kAnsi16,
  // The 256-color xterm palette.
  kAnsi256,
  // 24-bit direct (true) color.
  kTrueColor,
};

// Returns a stable, lowercase human-readable name for a color depth (used by
// examples and debug output).
const char* ColorDepthToName(ColorDepth depth) noexcept;

// A deterministic snapshot of what the terminal the process is running in is
// believed to support. This is a retention-safe value type: it owns every string
// it stores and holds no borrowed storage (no string_view, span, pointer, or
// reference). A default-constructed value is fully conservative -- nothing is
// advertised -- and is safe to use as an "unknown" fallback.
struct TerminalCapabilities {
  ColorDepth color_depth = ColorDepth::kNone;

  // UTF-8 / wide-glyph rendering support.
  bool unicode = false;
  // Mouse tracking (SGR 1006).
  bool mouse = false;
  // Bracketed paste (ESC [ ? 2004 h / l).
  bool bracketed_paste = false;
  // Hyperlink embedding (OSC 8).
  bool hyperlinks = false;
  // Kitty graphics protocol.
  bool kitty_graphics = false;
  // Sixel graphics protocol.
  bool sixel = false;
  // iTerm2 inline images (OSC 1337).
  bool iterm_images = false;
  // OSC 52 clipboard read/write.
  bool osc52 = false;
  // Alternate screen buffer (ESC [ ? 1049 h / l).
  bool alternate_screen = false;
  // Running inside a tmux session.
  bool tmux = false;
  // Running inside GNU screen.
  bool screen = false;
  // Session arrived over SSH.
  bool ssh = false;
  // Stable terminal identity: TERM_PROGRAM when set, otherwise TERM.
  std::string terminal_identity;

  // Value equality over every field.
  bool operator==(const TerminalCapabilities&) const = default;
};

}  // namespace terminal
}  // namespace terminal_ui_kit