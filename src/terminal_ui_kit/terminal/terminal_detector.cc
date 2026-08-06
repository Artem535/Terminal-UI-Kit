#include "terminal_ui_kit/terminal/terminal_detector.h"

#include <cctype>
#include <string>
#include <string_view>
#include <utility>

namespace terminal_ui_kit {
namespace terminal {
namespace {

// Lowercases ASCII and removes spaces; used for forgiving environment-value
// comparison (e.g. "WezTerm" -> "wezterm").
std::string Normalize(std::string_view value) {
  std::string result;
  result.reserve(value.size());
  for (const char c : value) {
    if (c == ' ') {
      continue;
    }
    result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  }
  return result;
}

// True when `value` equals `needle` or is `needle-...` (e.g. the TERM
// "xterm-256color" matches the base "xterm").
bool MatchesBase(std::string_view value, std::string_view needle) {
  if (value == needle) {
    return true;
  }
  return value.size() > needle.size() && value.compare(0, needle.size(), needle) == 0 &&
         value[needle.size()] == '-';
}

// True when `value` contains `needle` anywhere (used for program names and
// tone markers such as "256color").
bool Contains(std::string_view value, std::string_view needle) {
  return value.find(needle) != std::string_view::npos;
}

// Defaults for the boolean capabilities of a recognized terminal/program.
// Every field starts false (conservative) unless a profile enables it.
struct UxProfile {
  bool unicode;
  bool mouse;
  bool bracketed_paste;
  bool hyperlinks;
  bool kitty_graphics;
  bool sixel;
  bool iterm_images;
  bool osc52;
  bool alternate_screen;
};

// A recognition rule: matches when a recognized program name is present, or
// when the TERM base matches.
struct RecognitionRule {
  std::string_view needle;
  bool match_program;
  bool match_term_base;
  UxProfile profile;
};

using enum ColorDepth;

// Recognized terminals/programs, most specific first. The first rule whose
// TERM_PROGRAM or TERM matches wins and establishes the boolean baseline.
constexpr RecognitionRule kRecognitionRules[] = {
    {"kitty", true, true, {true, true, true, true, true, false, false, true, true}},
    {"ghostty", true, true, {true, true, true, true, false, true, false, true, true}},
    {"wezterm", true, true, {true, true, true, true, false, true, false, true, true}},
    {"foot", true, true, {true, true, true, false, false, true, false, true, true}},
    {"iterm", true, false, {true, true, true, true, false, true, true, true, true}},
    {"alacritty", true, true, {true, true, true, true, false, false, false, true, true}},
    {"vscode", true, false, {true, true, true, true, false, false, false, true, true}},
    {"apple", true, false, {true, true, true, true, false, false, false, true, true}},
    // TERM-base defaults for classic terminal classes.
    {"xterm", false, true, {true, true, true, true, false, false, false, true, true}},
    {"screen", false, true, {true, true, true, false, false, false, false, true, true}},
    {"tmux", false, true, {true, true, true, true, false, false, false, true, true}},
    {"rxvt", false, true, {true, false, true, false, false, false, false, true, true}},
    {"eterm", false, true, {true, true, true, false, false, false, false, true, true}},
    {"linux", false, true, {true, false, true, false, false, false, false, false, true}},
    {"vt100", false, true, {false, false, false, false, false, false, false, false, false}},
    {"vt220", false, true, {false, false, false, false, false, false, false, false, false}},
    {"ansi", false, true, {false, false, false, false, false, false, false, false, false}},
    {"dumb", false, true, {false, false, false, false, false, false, false, false, false}},
};

// Applies the first matching recognition rule to `caps` (boolean fields only).
// Returns true when a profile matched.
bool ApplyBaseline(TerminalCapabilities& caps, std::string_view program, std::string_view term) {
  for (const RecognitionRule& rule : kRecognitionRules) {
    if (rule.match_program && Contains(program, rule.needle)) {
      caps.unicode = rule.profile.unicode;
      caps.mouse = rule.profile.mouse;
      caps.bracketed_paste = rule.profile.bracketed_paste;
      caps.hyperlinks = rule.profile.hyperlinks;
      caps.kitty_graphics = rule.profile.kitty_graphics;
      caps.sixel = rule.profile.sixel;
      caps.iterm_images = rule.profile.iterm_images;
      caps.osc52 = rule.profile.osc52;
      caps.alternate_screen = rule.profile.alternate_screen;
      return true;
    }
    if (rule.match_term_base && MatchesBase(term, rule.needle)) {
      caps.unicode = rule.profile.unicode;
      caps.mouse = rule.profile.mouse;
      caps.bracketed_paste = rule.profile.bracketed_paste;
      caps.hyperlinks = rule.profile.hyperlinks;
      caps.kitty_graphics = rule.profile.kitty_graphics;
      caps.sixel = rule.profile.sixel;
      caps.iterm_images = rule.profile.iterm_images;
      caps.osc52 = rule.profile.osc52;
      caps.alternate_screen = rule.profile.alternate_screen;
      return true;
    }
  }
  return false;
}

// Stable, self-describing identity: a recognized program name wins, else the
// normalized TERM, else the normalized TERM_PROGRAM, else empty.
std::string ResolveIdentity(std::string_view program, std::string_view term,
                            std::string_view raw_program, std::string_view raw_term) {
  if (!program.empty()) {
    if (Contains(program, "kitty")) return "kitty";
    if (Contains(program, "ghostty")) return "ghostty";
    if (Contains(program, "wezterm")) return "wezterm";
    if (Contains(program, "foot")) return "foot";
    if (Contains(program, "iterm")) return "iterm2";
    if (Contains(program, "alacritty")) return "alacritty";
    if (Contains(program, "vscode")) return "vscode";
    if (Contains(program, "apple")) return "apple_terminal";
    // Unrecognized program: fall through and prefer TERM below.
  }
  if (!term.empty()) {
    return std::string(term);
  }
  if (!raw_program.empty()) {
    return std::string(raw_program);
  }
  (void)raw_term;
  return std::string();
}

// A 16-color-capable "classic" terminal base.
bool IsClassicColorBase(std::string_view term) {
  return MatchesBase(term, "xterm") || MatchesBase(term, "rxvt") || MatchesBase(term, "eterm") ||
         MatchesBase(term, "linux") || MatchesBase(term, "vt100") || MatchesBase(term, "vt220") ||
         MatchesBase(term, "ansi") || MatchesBase(term, "screen") || MatchesBase(term, "tmux");
}

// Applies a tri-state override to a detected boolean.
bool ResolveBool(bool detected, TriState state) {
  switch (state) {
    case TriState::kEnable:
      return true;
    case TriState::kDisable:
      return false;
    case TriState::kDefault:
      return detected;
  }
  return detected;
}

// Resolves color depth by precedence: override > NO_COLOR > COLORTERM >
// TERM-derived > conservative default.
ColorDepth ResolveColor(const CapabilityOverrides& overrides, bool has_no_color,
                        std::optional<std::string> colorterm, std::string_view term) {
  if (overrides.color_depth) {
    return *overrides.color_depth;
  }
  if (has_no_color) {
    return ColorDepth::kNone;
  }
  if (colorterm) {
    const std::string ncolor = Normalize(*colorterm);
    if (ncolor == "truecolor" || ncolor == "24bit") {
      return ColorDepth::kTrueColor;
    }
    if (ncolor == "256color") {
      return ColorDepth::k256Color;
    }
    if (ncolor == "16color") {
      return ColorDepth::k16Color;
    }
  }
  if (Contains(term, "256color")) {
    return ColorDepth::k256Color;
  }
  if (Contains(term, "16color")) {
    return ColorDepth::k16Color;
  }
  if (IsClassicColorBase(term)) {
    // TERM says nothing about depth but names a classic color-capable class.
    return ColorDepth::k16Color;
  }
  return ColorDepth::kNone;
}

}  // namespace

TerminalCapabilities TerminalDetector::Detect(const EnvironmentProvider& env,
                                              const CapabilityOverrides& overrides) const {
  const auto variable = [&env](const char* name) { return env.Get(name); };
  const std::optional<std::string> raw_term = variable("TERM");
  const std::optional<std::string> raw_program = variable("TERM_PROGRAM");
  const std::optional<std::string> colorterm = variable("COLORTERM");
  const bool has_no_color = variable("NO_COLOR").has_value();
  const std::optional<std::string> tmux = variable("TMUX");
  const std::optional<std::string> sty = variable("STY");
  const std::optional<std::string> ssh_tty = variable("SSH_TTY");
  const std::optional<std::string> ssh_client = variable("SSH_CLIENT");
  const std::optional<std::string> ssh_connection = variable("SSH_CONNECTION");

  const std::string term = raw_term ? Normalize(*raw_term) : std::string();
  const std::string program = raw_program ? Normalize(*raw_program) : std::string();

  TerminalCapabilities caps;

  // Container context: composable (tmux-over-SSH sets both tmux and ssh).
  caps.tmux = tmux.has_value() || Contains(term, "tmux");
  caps.screen = sty.has_value() || Contains(term, "screen");
  caps.ssh = ssh_tty.has_value() || ssh_client.has_value() || ssh_connection.has_value();

  // Recognized program / TERM preset establishes the boolean baseline.
  ApplyBaseline(caps, program, term);

  // Descriptive TERM tone markers that a base-name match cannot capture
  // (e.g. "xterm-256color-sixel" advertises sixel beyond a plain xterm).
  if (Contains(term, "sixel")) {
    caps.sixel = true;
  }

  caps.terminal_identity =
      ResolveIdentity(program, term, raw_program ? *raw_program : std::string_view(),
                      raw_term ? *raw_term : std::string_view());

  // Color depth, then explicit color override is resolved inside.
  caps.color_depth = ResolveColor(overrides, has_no_color, colorterm, term);

  // Explicit boolean overrides (highest precedence).
  caps.unicode = ResolveBool(caps.unicode, overrides.unicode);
  caps.mouse = ResolveBool(caps.mouse, overrides.mouse);
  caps.bracketed_paste = ResolveBool(caps.bracketed_paste, overrides.bracketed_paste);
  caps.hyperlinks = ResolveBool(caps.hyperlinks, overrides.hyperlinks);
  caps.kitty_graphics = ResolveBool(caps.kitty_graphics, overrides.kitty_graphics);
  caps.sixel = ResolveBool(caps.sixel, overrides.sixel);
  caps.iterm_images = ResolveBool(caps.iterm_images, overrides.iterm_images);
  caps.osc52 = ResolveBool(caps.osc52, overrides.osc52);
  caps.alternate_screen = ResolveBool(caps.alternate_screen, overrides.alternate_screen);

  return caps;
}

}  // namespace terminal
}  // namespace terminal_ui_kit