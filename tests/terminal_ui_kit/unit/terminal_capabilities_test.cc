#include <limits>
#include <map>
#include <string>
#include <utility>

#include "terminal_ui_kit/terminal/capabilities.h"
#include "terminal_ui_kit/terminal/detector.h"
#include "terminal_ui_kit/terminal/environment.h"
#include "terminal_ui_kit/terminal/overrides.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace terminal {
namespace {

// A deterministic environment snapshot backed by an in-memory map. Tests never
// read the real process environment.
class FakeEnvironment : public EnvironmentProvider {
 public:
  FakeEnvironment& Set(std::string name, std::string value) {
    vars_[std::move(name)] = std::move(value);
    return *this;
  }

  std::optional<std::string> Get(const std::string& name) const override {
    const auto it = vars_.find(name);
    if (it == vars_.end()) return std::nullopt;
    return it->second;
  }

 private:
  std::map<std::string, std::string, std::less<>> vars_;
};

TerminalCapabilities Detect(const FakeEnvironment& env) { return DetectTerminalCapabilities(env); }

// Shorthand for a real interactive terminal without any program-specific hints.
FakeEnvironment RealTerm() {
  FakeEnvironment env;
  env.Set("TERM", "xterm-256color");
  return env;
}

// ---------------------------------------------------------------------------
// Required environments
// ---------------------------------------------------------------------------

TEST(TerminalCapabilities, EmptyEnvironmentIsConservative) {
  const TerminalCapabilities caps = Detect(FakeEnvironment{});
  EXPECT_EQ(caps.color_depth, ColorDepth::kAnsi16);
  EXPECT_TRUE(caps.unicode);
  EXPECT_FALSE(caps.mouse);
  EXPECT_FALSE(caps.bracketed_paste);
  EXPECT_FALSE(caps.hyperlinks);
  EXPECT_FALSE(caps.kitty_graphics);
  EXPECT_FALSE(caps.sixel);
  EXPECT_FALSE(caps.iterm_images);
  EXPECT_FALSE(caps.osc52);
  EXPECT_FALSE(caps.alternate_screen);
  EXPECT_FALSE(caps.tmux);
  EXPECT_FALSE(caps.screen);
  EXPECT_FALSE(caps.ssh);
  EXPECT_TRUE(caps.terminal_identity.empty());
}

TEST(TerminalCapabilities, TermDumbDisablesAlmostEverything) {
  FakeEnvironment env;
  env.Set("TERM", "dumb");
  const TerminalCapabilities caps = Detect(env);
  EXPECT_EQ(caps.color_depth, ColorDepth::kNone);
  EXPECT_FALSE(caps.unicode);
  EXPECT_FALSE(caps.mouse);
  EXPECT_FALSE(caps.bracketed_paste);
  EXPECT_FALSE(caps.hyperlinks);
  EXPECT_FALSE(caps.alternate_screen);
  EXPECT_FALSE(caps.osc52);
  EXPECT_EQ(caps.terminal_identity, "dumb");
}

TEST(TerminalCapabilities, UnknownTerminalUsesConservativeBaseline) {
  FakeEnvironment env;
  env.Set("TERM", "unknownxyz");
  const TerminalCapabilities caps = Detect(env);
  EXPECT_EQ(caps.color_depth, ColorDepth::kAnsi16);
  EXPECT_TRUE(caps.unicode);
  EXPECT_TRUE(caps.mouse);
  EXPECT_TRUE(caps.bracketed_paste);
  EXPECT_TRUE(caps.hyperlinks);
  EXPECT_TRUE(caps.alternate_screen);
  EXPECT_TRUE(caps.osc52);
  EXPECT_EQ(caps.terminal_identity, "unknownxyz");
}

TEST(TerminalCapabilities, SixteenColorTerm) {
  FakeEnvironment env;
  env.Set("TERM", "xterm");
  const TerminalCapabilities caps = Detect(env);
  EXPECT_EQ(caps.color_depth, ColorDepth::kAnsi16);
}

TEST(TerminalCapabilities, TwoFiftySixColorTerm) {
  FakeEnvironment env;
  env.Set("TERM", "xterm-256color");
  const TerminalCapabilities caps = Detect(env);
  EXPECT_EQ(caps.color_depth, ColorDepth::kAnsi256);
}

TEST(TerminalCapabilities, TrueColorViaColorterm) {
  for (const char* value : {"truecolor", "24bit", "TrueColor"}) {
    FakeEnvironment env;
    env.Set("TERM", "xterm");
    env.Set("COLORTERM", value);
    EXPECT_EQ(Detect(env).color_depth, ColorDepth::kTrueColor);
  }
}

TEST(TerminalCapabilities, TrueColorViaTermHint) {
  for (const char* term : {"xterm-direct", "alacritty-direct", "xterm-truecolor"}) {
    FakeEnvironment env;
    env.Set("TERM", term);
    EXPECT_EQ(Detect(env).color_depth, ColorDepth::kTrueColor);
  }
}

TEST(TerminalCapabilities, KittyPreset) {
  FakeEnvironment env;
  env.Set("TERM", "xterm-kitty");
  env.Set("TERM_PROGRAM", "kitty");
  env.Set("COLORTERM", "truecolor");
  const TerminalCapabilities caps = Detect(env);
  EXPECT_EQ(caps.color_depth, ColorDepth::kTrueColor);
  EXPECT_TRUE(caps.unicode);
  EXPECT_TRUE(caps.kitty_graphics);
  EXPECT_TRUE(caps.sixel);
  EXPECT_EQ(caps.terminal_identity, "kitty");
}

TEST(TerminalCapabilities, Iterm2Preset) {
  FakeEnvironment env;
  env.Set("TERM", "xterm-256color");
  env.Set("TERM_PROGRAM", "iTerm.app");
  env.Set("TERM_PROGRAM_VERSION", "3.4.19");
  env.Set("COLORTERM", "truecolor");
  const TerminalCapabilities caps = Detect(env);
  EXPECT_EQ(caps.color_depth, ColorDepth::kTrueColor);
  EXPECT_TRUE(caps.iterm_images);
  EXPECT_TRUE(caps.sixel);
  EXPECT_TRUE(caps.osc52);
  EXPECT_EQ(caps.terminal_identity, "iTerm.app");
}

TEST(TerminalCapabilities, ItermImagesRequiresVersionAtLeast3) {
  for (const char* version : {"2.0", "1", "3", "3.1"}) {
    FakeEnvironment env;
    env.Set("TERM_PROGRAM", "iTerm.app");
    env.Set("TERM_PROGRAM_VERSION", version);
    EXPECT_EQ(Detect(env).iterm_images, std::stoi(version) >= 3) << "version=" << version;
  }
}

TEST(TerminalCapabilities, SixelEnabledOnlyForKnownPrograms) {
  for (const char* program : {"wezterm", "xterm", "kitty", "iTerm.app"}) {
    FakeEnvironment env;
    env.Set("TERM", "xterm");
    env.Set("TERM_PROGRAM", program);
    EXPECT_TRUE(Detect(env).sixel) << "program=" << program;
  }
  FakeEnvironment unknown;
  unknown.Set("TERM", "xterm");
  unknown.Set("TERM_PROGRAM", "SomeRandomTerminal");
  EXPECT_FALSE(Detect(unknown).sixel);
}

TEST(TerminalCapabilities, Osc52DisabledForAppleTerminal) {
  FakeEnvironment env;
  env.Set("TERM", "xterm-256color");
  env.Set("TERM_PROGRAM", "Apple_Terminal");
  const TerminalCapabilities caps = Detect(env);
  EXPECT_FALSE(caps.osc52);
  EXPECT_TRUE(caps.hyperlinks);
  EXPECT_TRUE(caps.unicode);
}

TEST(TerminalCapabilities, TmuxPresent) {
  FakeEnvironment env = RealTerm();
  env.Set("TMUX", "/tmp/tmux-1000/default,12345,0");
  const TerminalCapabilities caps = Detect(env);
  EXPECT_TRUE(caps.tmux);
  EXPECT_FALSE(caps.screen);
  EXPECT_FALSE(caps.ssh);
}

TEST(TerminalCapabilities, GnuScreenPresent) {
  FakeEnvironment env = RealTerm();
  env.Set("STY", "12345.pts-0.host");
  const TerminalCapabilities caps = Detect(env);
  EXPECT_TRUE(caps.screen);
  EXPECT_FALSE(caps.tmux);
}

TEST(TerminalCapabilities, SshPresentThroughAnySignal) {
  for (const char* name : {"SSH_CONNECTION", "SSH_CLIENT", "SSH_TTY"}) {
    FakeEnvironment env = RealTerm();
    env.Set(name, "sample-value");
    EXPECT_TRUE(Detect(env).ssh) << "signal=" << name;
  }
}

TEST(TerminalCapabilities, TmuxOverSsh) {
  FakeEnvironment env;
  env.Set("TERM", "tmux-256color");
  env.Set("TMUX", "/tmp/tmux-1000/default,12345,0");
  env.Set("SSH_CONNECTION", "192.168.1.10 54321 192.168.1.1 22");
  const TerminalCapabilities caps = Detect(env);
  EXPECT_TRUE(caps.tmux);
  EXPECT_TRUE(caps.ssh);
  EXPECT_EQ(caps.color_depth, ColorDepth::kAnsi256);
  EXPECT_EQ(caps.terminal_identity, "tmux-256color");
}

TEST(TerminalCapabilities, NestedTmuxScreenAndSshAllReported) {
  FakeEnvironment env;
  env.Set("TERM", "xterm-256color");
  env.Set("TMUX", "/tmp/tmux-1000/default,1,0");
  env.Set("STY", "2.pts-0.host");
  env.Set("SSH_TTY", "/dev/pts/0");
  const TerminalCapabilities caps = Detect(env);
  EXPECT_TRUE(caps.tmux);
  EXPECT_TRUE(caps.screen);
  EXPECT_TRUE(caps.ssh);
}

// ---------------------------------------------------------------------------
// NO_COLOR and conflicts
// ---------------------------------------------------------------------------

TEST(TerminalCapabilities, NoColorDisablesColorOnly) {
  FakeEnvironment env;
  env.Set("TERM", "xterm-256color");
  env.Set("NO_COLOR", "1");
  const TerminalCapabilities caps = Detect(env);
  EXPECT_EQ(caps.color_depth, ColorDepth::kNone);
  EXPECT_TRUE(caps.unicode);
  EXPECT_TRUE(caps.mouse);
  EXPECT_TRUE(caps.bracketed_paste);
  EXPECT_TRUE(caps.hyperlinks);
  EXPECT_TRUE(caps.osc52);
  EXPECT_TRUE(caps.alternate_screen);
}

TEST(TerminalCapabilities, NoColorEmptyDoesNotDisableColor) {
  FakeEnvironment env = RealTerm();
  env.Set("NO_COLOR", "");
  const TerminalCapabilities caps = Detect(env);
  EXPECT_EQ(caps.color_depth, ColorDepth::kAnsi256);
}

TEST(TerminalCapabilities, ColortermWinsOverTermHints) {
  FakeEnvironment env;
  env.Set("TERM", "xterm");
  env.Set("COLORTERM", "truecolor");
  EXPECT_EQ(Detect(env).color_depth, ColorDepth::kTrueColor);
}

TEST(TerminalCapabilities, NoColorBeatsTrueColorHints) {
  FakeEnvironment env = RealTerm();
  env.Set("COLORTERM", "truecolor");
  env.Set("NO_COLOR", "1");
  EXPECT_EQ(Detect(env).color_depth, ColorDepth::kNone);
}

// ---------------------------------------------------------------------------
// Missing variables and malformed versions
// ---------------------------------------------------------------------------

TEST(TerminalCapabilities, UnknownTermProgramKeepsTermIdentity) {
  FakeEnvironment env = RealTerm();
  env.Set("TERM_PROGRAM", "SomeRandomApp");
  const TerminalCapabilities caps = Detect(env);
  EXPECT_EQ(caps.terminal_identity, "SomeRandomApp");
  EXPECT_EQ(caps.color_depth, ColorDepth::kAnsi256);
  EXPECT_FALSE(caps.kitty_graphics);
  EXPECT_FALSE(caps.sixel);
  EXPECT_FALSE(caps.iterm_images);
  EXPECT_EQ(caps.osc52, true);  // unknown program does not disable OSC 52
}

TEST(TerminalCapabilities, MissingVariablesFallBackToTerm) {
  FakeEnvironment env;
  env.Set("TERM", "xterm-256color");
  EXPECT_EQ(Detect(env).terminal_identity, "xterm-256color");
}

TEST(TerminalCapabilities, MalformedVersionKeepsItermImagesOff) {
  FakeEnvironment env;
  env.Set("TERM_PROGRAM", "iTerm.app");
  env.Set("TERM_PROGRAM_VERSION", "not-a-version");
  env.Set("TERM", "xterm");
  const TerminalCapabilities caps = Detect(env);
  EXPECT_FALSE(caps.iterm_images);
  EXPECT_EQ(caps.terminal_identity, "iTerm.app");
  // Other capabilities are unaffected by the malformed version.
  EXPECT_EQ(caps.color_depth, ColorDepth::kAnsi16);
  EXPECT_TRUE(caps.unicode);
}

TEST(ParseProgramVersion, ParsesLeadingNumbers) {
  int major = -1;
  int minor = -1;
  EXPECT_TRUE(ParseProgramVersion("3.4.19", major, minor));
  EXPECT_EQ(major, 3);
  EXPECT_EQ(minor, 4);
  EXPECT_TRUE(ParseProgramVersion("3", major, minor));
  EXPECT_EQ(major, 3);
  EXPECT_EQ(minor, 0);
  EXPECT_TRUE(ParseProgramVersion("3.4.19 (develop)", major, minor));
  EXPECT_EQ(major, 3);
  EXPECT_EQ(minor, 4);
}

TEST(ParseProgramVersion, RejectsMalformedInput) {
  int major = -1;
  int minor = -1;
  EXPECT_FALSE(ParseProgramVersion("", major, minor));
  EXPECT_EQ(major, 0);
  EXPECT_EQ(minor, 0);
  EXPECT_FALSE(ParseProgramVersion("abc", major, minor));
  EXPECT_FALSE(ParseProgramVersion("-1.0", major, minor));
  EXPECT_FALSE(ParseProgramVersion("   ", major, minor));
}

TEST(ParseProgramVersion, HugeNumberSaturatesWithoutOverflow) {
  // A very long digit run must not overflow into undefined behavior; it
  // saturates at the int maximum and parses without crashing.
  int major = -1;
  int minor = -1;
  const std::string huge = "999999999999999999999999999999";
  EXPECT_TRUE(ParseProgramVersion(huge, major, minor));
  EXPECT_EQ(major, std::numeric_limits<int>::max());
  EXPECT_EQ(minor, 0);
  const std::string huge_minor = "3.999999999999999999999999999";
  EXPECT_TRUE(ParseProgramVersion(huge_minor, major, minor));
  EXPECT_EQ(major, 3);
  EXPECT_EQ(minor, std::numeric_limits<int>::max());
}

// ---------------------------------------------------------------------------
// Stable identity
// ---------------------------------------------------------------------------

TEST(TerminalCapabilities, IdentityIsStableAcrossDetections) {
  FakeEnvironment env = RealTerm();
  env.Set("TERM_PROGRAM", "iTerm.app");
  const TerminalCapabilities first = Detect(env);
  const TerminalCapabilities second = Detect(env);
  EXPECT_EQ(first.terminal_identity, "iTerm.app");
  EXPECT_EQ(second.terminal_identity, first.terminal_identity);
  // Same environment yields a fully equal snapshot.
  EXPECT_TRUE(first == second);
}

// ---------------------------------------------------------------------------
// Explicit overrides
// ---------------------------------------------------------------------------

TEST(ApplyOverrides, ExplicitEnableAndDisableWin) {
  const TerminalCapabilities detected = Detect(RealTerm());
  CapabilityOverrides overrides;
  overrides.color_depth = ColorDepth::kTrueColor;
  overrides.mouse = false;  // disable
  overrides.sixel = true;   // enable
  overrides.terminal_identity = "forced";
  const TerminalCapabilities resolved = ApplyOverrides(detected, overrides);

  EXPECT_EQ(resolved.color_depth, ColorDepth::kTrueColor);
  EXPECT_FALSE(resolved.mouse);
  EXPECT_TRUE(resolved.sixel);
  EXPECT_EQ(resolved.terminal_identity, "forced");
  // Non-overridden fields are preserved.
  EXPECT_TRUE(resolved.unicode);
  EXPECT_TRUE(resolved.bracketed_paste);
}

TEST(ApplyOverrides, DisableOverrideBeatsDumbOrHint) {
  // Enable unicode on a dumb terminal, and enable a color depth that NO_COLOR
  // suppressed -- overrides always win.
  FakeEnvironment env;
  env.Set("TERM", "xterm-256color");
  env.Set("NO_COLOR", "1");
  const TerminalCapabilities detected = Detect(env);
  EXPECT_EQ(detected.color_depth, ColorDepth::kNone);

  CapabilityOverrides overrides;
  overrides.color_depth = ColorDepth::kAnsi256;
  overrides.unicode = true;
  const TerminalCapabilities resolved = ApplyOverrides(detected, overrides);
  EXPECT_EQ(resolved.color_depth, ColorDepth::kAnsi256);
  EXPECT_TRUE(resolved.unicode);
}

TEST(ApplyOverrides, EmptyOverridesIsNoOp) {
  const TerminalCapabilities detected = Detect(RealTerm());
  EXPECT_TRUE(ApplyOverrides(detected, CapabilityOverrides{}) == detected);
}

TEST(ResolveTerminalCapabilities, AppliesOverridesInOneStep) {
  FakeEnvironment env;
  env.Set("TERM", "xterm");
  CapabilityOverrides overrides;
  overrides.osc52 = false;
  const TerminalCapabilities caps = ResolveTerminalCapabilities(env, overrides);
  EXPECT_FALSE(caps.osc52);
  EXPECT_EQ(caps.color_depth, ColorDepth::kAnsi16);
}

}  // namespace
}  // namespace terminal
}  // namespace terminal_ui_kit