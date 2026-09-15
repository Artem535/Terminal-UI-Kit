#include "terminal_ui_kit/terminal/overrides.h"

namespace terminal_ui_kit {
namespace terminal {

TerminalCapabilities ApplyOverrides(const TerminalCapabilities& caps,
                                    const CapabilityOverrides& overrides) {
  TerminalCapabilities result = caps;
  if (overrides.color_depth.has_value()) result.color_depth = *overrides.color_depth;
  if (overrides.unicode.has_value()) result.unicode = *overrides.unicode;
  if (overrides.mouse.has_value()) result.mouse = *overrides.mouse;
  if (overrides.bracketed_paste.has_value()) {
    result.bracketed_paste = *overrides.bracketed_paste;
  }
  if (overrides.hyperlinks.has_value()) result.hyperlinks = *overrides.hyperlinks;
  if (overrides.kitty_graphics.has_value()) result.kitty_graphics = *overrides.kitty_graphics;
  if (overrides.sixel.has_value()) result.sixel = *overrides.sixel;
  if (overrides.iterm_images.has_value()) result.iterm_images = *overrides.iterm_images;
  if (overrides.osc52.has_value()) result.osc52 = *overrides.osc52;
  if (overrides.alternate_screen.has_value()) {
    result.alternate_screen = *overrides.alternate_screen;
  }
  if (overrides.tmux.has_value()) result.tmux = *overrides.tmux;
  if (overrides.screen.has_value()) result.screen = *overrides.screen;
  if (overrides.ssh.has_value()) result.ssh = *overrides.ssh;
  if (overrides.terminal_identity.has_value()) {
    result.terminal_identity = *overrides.terminal_identity;
  }
  return result;
}

}  // namespace terminal
}  // namespace terminal_ui_kit