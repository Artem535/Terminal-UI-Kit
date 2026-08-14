#pragma once

#include <optional>
#include <string>

#include "terminal_ui_kit/terminal/capabilities.h"

namespace terminal_ui_kit {
namespace terminal {

// Explicit per-capability user overrides. An engaged (has_value) field always
// wins over the value produced by detection; a disengaged field leaves the
// detected value intact. This single mechanism provides both an explicit
// *enable* override (true) and an explicit *disable* override (false). A
// default-constructed value means "no overrides" and is a no-op.
struct CapabilityOverrides {
  std::optional<ColorDepth> color_depth;
  std::optional<bool> unicode;
  std::optional<bool> mouse;
  std::optional<bool> bracketed_paste;
  std::optional<bool> hyperlinks;
  std::optional<bool> kitty_graphics;
  std::optional<bool> sixel;
  std::optional<bool> iterm_images;
  std::optional<bool> osc52;
  std::optional<bool> alternate_screen;
  std::optional<bool> tmux;
  std::optional<bool> screen;
  std::optional<bool> ssh;
  std::optional<std::string> terminal_identity;
};

// Applies `overrides` on top of the detected `caps`. Explicit overrides take
// precedence over everything detection derived from the environment.
TerminalCapabilities ApplyOverrides(const TerminalCapabilities& caps,
                                    const CapabilityOverrides& overrides);

}  // namespace terminal
}  // namespace terminal_ui_kit