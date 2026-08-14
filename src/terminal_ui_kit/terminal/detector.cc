#include "terminal_ui_kit/terminal/detector.h"

#include <cstddef>
#include <limits>
#include <string>

namespace terminal_ui_kit {
namespace terminal {
namespace {

// Returns true when the variable is set to a non-empty string.
bool PresentNonEmpty(const EnvironmentProvider& env, const std::string& name) {
  const std::optional<std::string> value = env.Get(name);
  return value.has_value() && !value->empty();
}

// Lowercases a string in place (ASCII only; environment signals are ASCII).
void ToLowerInPlace(std::string& s) {
  for (char& c : s) {
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
  }
}

// Returns true when `haystack` (already lowercased) contains `needle`.
bool ContainsLower(const std::string& haystack, std::string_view needle) {
  return haystack.find(needle) != std::string::npos;
}

// The terminal is a real interactive terminal: TERM is set and not "dumb".
// Several interaction features are only claimed for such a terminal.
bool HasRealTerminal(const std::string& term_lower) {
  return !term_lower.empty() && term_lower != "dumb";
}

// Parses a leading run of ASCII digits starting at `*pos` into a saturating
// value, advancing `*pos` past the consumed digits. Returns false when *pos is
// not on a digit. The accumulation never overflows: each step is guarded, and
// an over-long run saturates at the maximum rather than invoking undefined
// behavior (a maliciously huge version string is treated as "very large").
bool ParseLeadingDigits(std::string_view raw, std::size_t& pos, int& out) {
  if (pos >= raw.size() || raw[pos] < '0' || raw[pos] > '9') return false;
  long long value = 0;
  static constexpr long long kMax = std::numeric_limits<int>::max();
  while (pos < raw.size() && raw[pos] >= '0' && raw[pos] <= '9') {
    const int digit = raw[pos] - '0';
    if (value > (kMax - digit) / 10) {
      // Saturate; keep consuming so *pos advances past the whole run.
      do {
        ++pos;
      } while (pos < raw.size() && raw[pos] >= '0' && raw[pos] <= '9');
      out = static_cast<int>(kMax);
      return true;
    }
    value = value * 10 + digit;
    ++pos;
  }
  out = static_cast<int>(value);
  return true;
}

}  // namespace

bool ParseProgramVersion(std::string_view raw, int& major, int& minor) noexcept {
  major = 0;
  minor = 0;

  std::size_t pos = 0;
  while (pos < raw.size() && (raw[pos] == ' ' || raw[pos] == '\t')) ++pos;
  if (!ParseLeadingDigits(raw, pos, major)) return false;
  if (pos < raw.size() && raw[pos] == '.') {
    ++pos;
    ParseLeadingDigits(raw, pos, minor);
  }
  return true;
}

TerminalCapabilities DetectTerminalCapabilities(const EnvironmentProvider& env) {
  TerminalCapabilities caps;

  const std::string term = env.Get("TERM").value_or("");
  std::string term_lower = term;
  ToLowerInPlace(term_lower);
  const std::string colorterm_lower = [&] {
    std::string value = env.Get("COLORTERM").value_or("");
    ToLowerInPlace(value);
    return value;
  }();
  const std::string term_program_lower = [&] {
    std::string value = env.Get("TERM_PROGRAM").value_or("");
    ToLowerInPlace(value);
    return value;
  }();
  const std::string term_program_version = env.Get("TERM_PROGRAM_VERSION").value_or("");

  const bool no_color = PresentNonEmpty(env, "NO_COLOR");
  const bool dumb = term_lower == "dumb";
  const bool real_terminal = HasRealTerminal(term_lower);

  // ---- Stable identity: TERM_PROGRAM, else TERM. -------------------------
  if (const std::optional<std::string> program = env.Get("TERM_PROGRAM");
      program.has_value() && !program->empty()) {
    caps.terminal_identity = *program;
  } else if (!term.empty()) {
    caps.terminal_identity = term;
  }

  // ---- Session flags are direct presence signals. ------------------------
  caps.tmux = PresentNonEmpty(env, "TMUX");
  caps.screen = PresentNonEmpty(env, "STY");
  caps.ssh = PresentNonEmpty(env, "SSH_CONNECTION") || PresentNonEmpty(env, "SSH_CLIENT") ||
             PresentNonEmpty(env, "SSH_TTY");

  // ---- Color depth. Precedence: NO_COLOR/TERM=dumb, COLORTERM, TERM hints. --
  if (no_color || dumb) {
    caps.color_depth = ColorDepth::kNone;
  } else if (colorterm_lower == "truecolor" || colorterm_lower == "24bit") {
    caps.color_depth = ColorDepth::kTrueColor;
  } else if (!colorterm_lower.empty()) {
    caps.color_depth = ColorDepth::kAnsi256;
  } else if (ContainsLower(term_lower, "truecolor") || ContainsLower(term_lower, "direct")) {
    caps.color_depth = ColorDepth::kTrueColor;
  } else if (ContainsLower(term_lower, "256color")) {
    caps.color_depth = ColorDepth::kAnsi256;
  } else if (ContainsLower(term_lower, "color")) {
    caps.color_depth = ColorDepth::kAnsi16;
  } else {
    // Conservative baseline for an unknown or empty TERM: the universal ANSI
    // minimum rather than claiming 256 or truecolor.
    caps.color_depth = ColorDepth::kAnsi16;
  }

  // ---- Unicode: effectively universal; only dumb terminals cannot render it.
  caps.unicode = !dumb;

  // ---- Interaction features are claimed only for a real terminal. ---------
  caps.mouse = real_terminal;
  caps.bracketed_paste = real_terminal;
  caps.hyperlinks = real_terminal;
  caps.alternate_screen = real_terminal;
  // OSC 52 is not implemented by Apple Terminal.
  caps.osc52 = real_terminal && term_program_lower != "apple_terminal";

  // ---- Program-specific image protocols (opt-in by identification). --------
  caps.kitty_graphics = term_program_lower == "kitty" || ContainsLower(term_lower, "xterm-kitty");

  // iTerm2 inline images (OSC 1337) are advertised only when the program is
  // identified and its version is parseable and at least 3 (where images
  // arrived); a missing or malformed version conservatively disables them.
  int major = 0;
  int minor = 0;
  const bool version_ok = ParseProgramVersion(term_program_version, major, minor);
  (void)minor;
  caps.iterm_images = term_program_lower == "iterm.app" && version_ok && major >= 3;

  caps.sixel = term_program_lower == "wezterm" || term_program_lower == "xterm" ||
               term_program_lower == "kitty" || term_program_lower == "iterm.app" ||
               ContainsLower(term_lower, "sixel");

  return caps;
}

TerminalCapabilities ResolveTerminalCapabilities(const EnvironmentProvider& env,
                                                 const CapabilityOverrides& overrides) {
  return ApplyOverrides(DetectTerminalCapabilities(env), overrides);
}

}  // namespace terminal
}  // namespace terminal_ui_kit