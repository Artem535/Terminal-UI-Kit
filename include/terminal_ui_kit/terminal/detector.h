#pragma once

#include <string_view>

#include "terminal_ui_kit/terminal/capabilities.h"
#include "terminal_ui_kit/terminal/environment.h"
#include "terminal_ui_kit/terminal/overrides.h"

namespace terminal_ui_kit {
namespace terminal {

// Parses a program version string such as "3.4.19", "3", or "3.4.19 (develop)"
// into its leading major and minor integers. Returns true and writes both
// values when the string begins with a non-negative integer; otherwise returns
// false and writes 0 for both. Trailing non-integer characters are ignored, so
// the parser never throws and accepts mixed input. Used by the detection policy
// to gate version-dependent features conservatively: an empty or malformed
// version indicates "unknown" and disables the gated feature.
bool ParseProgramVersion(std::string_view raw, int& major, int& minor) noexcept;

// Detects terminal capabilities from `env`. Detection is a pure function of the
// provider and fully deterministic; it uses conservative defaults whenever a
// signal is missing or ambiguous (see the design spec for the exact precedence
// and conventions).
TerminalCapabilities DetectTerminalCapabilities(const EnvironmentProvider& env);

// Convenience combining DetectTerminalCapabilities() with ApplyOverrides().
TerminalCapabilities ResolveTerminalCapabilities(const EnvironmentProvider& env,
                                                 const CapabilityOverrides& overrides);

}  // namespace terminal
}  // namespace terminal_ui_kit