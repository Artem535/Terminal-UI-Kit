#pragma once

#include <optional>

#include "terminal_ui_kit/terminal/environment.h"
#include "terminal_ui_kit/terminal/terminal_capabilities.h"

namespace terminal_ui_kit {
namespace terminal {

// A tri-state flag used by CapabilityOverrides: an explicit decision
// (kEnable / kDisable) or no decision (kDefault, meaning "derive from the
// environment").
enum class TriState {
  kDefault,
  kEnable,
  kDisable,
};

// Explicit user overrides applied on top of environment detection. Every
// field defaults to kDefault / nullopt, so an override only ever changes the
// specific capabilities it names. An explicit kEnable / kDisable wins over
// every environment signal, including NO_COLOR.
struct CapabilityOverrides {
  TriState unicode = TriState::kDefault;
  TriState mouse = TriState::kDefault;
  TriState bracketed_paste = TriState::kDefault;
  TriState hyperlinks = TriState::kDefault;
  TriState kitty_graphics = TriState::kDefault;
  TriState sixel = TriState::kDefault;
  TriState iterm_images = TriState::kDefault;
  TriState osc52 = TriState::kDefault;
  TriState alternate_screen = TriState::kDefault;
  // When set, pins color_depth; otherwise derived from the environment.
  std::optional<ColorDepth> color_depth = std::nullopt;
};

// Deterministic, side-effect-free terminal-capability detector (PRD section
// 63, "terminal integrations"). Detection reads environment variables through
// a caller-supplied EnvironmentProvider and combines them with explicit
// overrides. It never probes a live terminal and never reads stdin/stdout.
//
// Precedence, lowest to highest:
//   1. Conservative defaults (every flag off, color kNone).
//   2. Container context: tmux / screen / ssh.
//   3. Recognized program / TERM preset (color, unicode, mouse, ...).
//   4. COLORTERM (may raise color depth above the TERM guess).
//   5. NO_COLOR (lowers color depth to kNone; no other flag is affected).
//   6. Explicit CapabilityOverrides (may raise or lower anything).
class TerminalDetector {
 public:
  // Detects capabilities for the given environment provider and overrides.
  [[nodiscard]] TerminalCapabilities Detect(const EnvironmentProvider& env,
                                            const CapabilityOverrides& overrides) const;
};

}  // namespace terminal
}  // namespace terminal_ui_kit