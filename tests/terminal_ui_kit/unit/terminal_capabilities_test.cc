#include "terminal_ui_kit/terminal/terminal_capabilities.h"

#include <string>
#include <unordered_map>

#include "terminal_ui_kit/terminal/environment.h"
#include "terminal_ui_kit/terminal/terminal_detector.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace terminal {
namespace {

using terminal_ui_kit::terminal::CapabilityOverrides;
using terminal_ui_kit::terminal::ColorDepth;
using terminal_ui_kit::terminal::MapEnvironment;
using terminal_ui_kit::terminal::TerminalCapabilities;
using terminal_ui_kit::terminal::TerminalDetector;
using terminal_ui_kit::terminal::TriState;

TerminalCapabilities Detect(std::unordered_map<std::string, std::string> variables,
                            const CapabilityOverrides& overrides = {}) {
  const MapEnvironment env(std::move(variables));
  return TerminalDetector{}.Detect(env, overrides);
}

// Returns a capabilities for a plain xterm-256color (the common baseline used
// by many tests below).
TerminalCapabilities Xterm256() { return Detect({{"TERM", "xterm-256color"}}); }

// xterm-256color with the given overrides applied.
TerminalCapabilities Xterm256WithOverrides(const CapabilityOverrides& overrides) {
  return Detect({{"TERM", "xterm-256color"}}, overrides);
}

TEST(TerminalCapabilities, EmptyEnvironmentIsConservative) {
  const TerminalCapabilities caps = Detect({});
  EXPECT_EQ(caps.color_depth, ColorDepth::kNone);
  EXPECT_FALSE(caps.unicode);
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

TEST(TerminalCapabilities, DumbTerminalIsMinimal) {
  const TerminalCapabilities caps = Detect({{"TERM", "dumb"}});
  EXPECT_EQ(caps.color_depth, ColorDepth::kNone);
  EXPECT_FALSE(caps.unicode);
  EXPECT_FALSE(caps.mouse);
  EXPECT_FALSE(caps.bracketed_paste);
  EXPECT_FALSE(caps.hyperlinks);
  EXPECT_FALSE(caps.kitty_graphics);
  EXPECT_FALSE(caps.sixel);
  EXPECT_FALSE(caps.iterm_images);
  EXPECT_FALSE(caps.osc52);
  EXPECT_FALSE(caps.alternate_screen);
  EXPECT_EQ(caps.terminal_identity, "dumb");
}

TEST(TerminalCapabilities, UnknownTerminalIsConservative) {
  const TerminalCapabilities caps = Detect({{"TERM", "weird-nobody-knows"}});
  EXPECT_EQ(caps.color_depth, ColorDepth::kNone);
  EXPECT_FALSE(caps.unicode);
  EXPECT_FALSE(caps.mouse);
  EXPECT_FALSE(caps.hyperlinks);
  EXPECT_FALSE(caps.alternate_screen);
  // Identity is still self-describing and predictable.
  EXPECT_EQ(caps.terminal_identity, "weird-nobody-knows");
}

TEST(TerminalCapabilities, Detects16Color) {
  const TerminalCapabilities caps = Detect({{"TERM", "xterm"}});
  EXPECT_EQ(caps.color_depth, ColorDepth::k16Color);
  EXPECT_TRUE(caps.unicode);
}

TEST(TerminalCapabilities, Detects16ColorFromColorterm) {
  const TerminalCapabilities caps = Detect({{"TERM", "xterm-256color"}, {"COLORTERM", "16color"}});
  EXPECT_EQ(caps.color_depth, ColorDepth::k16Color);
}

TEST(TerminalCapabilities, Detects256Color) {
  const TerminalCapabilities caps = Detect({{"TERM", "xterm-256color"}});
  EXPECT_EQ(caps.color_depth, ColorDepth::k256Color);
  EXPECT_EQ(caps.terminal_identity, "xterm-256color");
}

TEST(TerminalCapabilities, Detects256ColorFromColorterm) {
  const TerminalCapabilities caps = Detect({{"COLORTERM", "256color"}});
  EXPECT_EQ(caps.color_depth, ColorDepth::k256Color);
}

TEST(TerminalCapabilities, DetectsTrueColor) {
  const TerminalCapabilities caps =
      Detect({{"TERM", "xterm-256color"}, {"COLORTERM", "truecolor"}});
  EXPECT_EQ(caps.color_depth, ColorDepth::kTrueColor);
}

TEST(TerminalCapabilities, DetectsTrueColorFrom24bit) {
  const TerminalCapabilities caps = Detect({{"COLORTERM", "24bit"}});
  EXPECT_EQ(caps.color_depth, ColorDepth::kTrueColor);
}

TEST(TerminalCapabilities, ColortermWinsOverTermDepth) {
  // COLORTERM=truecolor must beat a TERM that only promises 256 colours.
  const TerminalCapabilities caps =
      Detect({{"TERM", "xterm-256color"}, {"COLORTERM", "truecolor"}});
  EXPECT_EQ(caps.color_depth, ColorDepth::kTrueColor);
}

TEST(TerminalCapabilities, UnicodeEnabledForModernTerminal) { EXPECT_TRUE(Xterm256().unicode); }

TEST(TerminalCapabilities, KittyTerminal) {
  const TerminalCapabilities caps =
      Detect({{"TERM", "xterm-256color"}, {"COLORTERM", "truecolor"}, {"TERM_PROGRAM", "kitty"}});
  EXPECT_EQ(caps.color_depth, ColorDepth::kTrueColor);
  EXPECT_TRUE(caps.kitty_graphics);
  EXPECT_TRUE(caps.hyperlinks);
  EXPECT_TRUE(caps.bracketed_paste);
  EXPECT_TRUE(caps.osc52);
  EXPECT_EQ(caps.terminal_identity, "kitty");
}

TEST(TerminalCapabilities, Iterm2Terminal) {
  const TerminalCapabilities caps = Detect(
      {{"TERM", "xterm-256color"}, {"COLORTERM", "truecolor"}, {"TERM_PROGRAM", "iTerm.app"}});
  EXPECT_TRUE(caps.iterm_images);
  EXPECT_TRUE(caps.sixel);
  EXPECT_TRUE(caps.hyperlinks);
  EXPECT_EQ(caps.terminal_identity, "iterm2");
}

TEST(TerminalCapabilities, WeztermEnablesSixelAndImages) {
  const TerminalCapabilities caps =
      Detect({{"TERM", "xterm-256color"}, {"TERM_PROGRAM", "WezTerm"}});
  EXPECT_TRUE(caps.sixel);
  EXPECT_TRUE(caps.hyperlinks);
  EXPECT_TRUE(caps.osc52);
  EXPECT_EQ(caps.terminal_identity, "wezterm");
}

TEST(TerminalCapabilities, SixelAdvertisedViaTermTone) {
  const TerminalCapabilities caps = Detect({{"TERM", "xterm-256color-sixel"}});
  EXPECT_TRUE(caps.sixel);
}

TEST(TerminalCapabilities, ScreenMultiplexer) {
  const TerminalCapabilities caps = Detect({{"STY", "12345.pts-0"}});
  EXPECT_TRUE(caps.screen);
  EXPECT_FALSE(caps.tmux);
  EXPECT_FALSE(caps.ssh);
}

TEST(TerminalCapabilities, TmuxMultiplexer) {
  const TerminalCapabilities caps = Detect({{"TMUX", "/tmp/tmux-0/default,1,0"}});
  EXPECT_TRUE(caps.tmux);
  EXPECT_FALSE(caps.screen);
  EXPECT_FALSE(caps.ssh);
}

TEST(TerminalCapabilities, SshConnection) {
  const TerminalCapabilities caps = Detect({{"SSH_CONNECTION", "1.2.3.4 51234 5.6.7.8 22"}});
  EXPECT_TRUE(caps.ssh);
  EXPECT_FALSE(caps.tmux);
  EXPECT_FALSE(caps.screen);
}

TEST(TerminalCapabilities, SshClientAndTtyAlsoCount) {
  EXPECT_TRUE(Detect({{"SSH_CLIENT", "1.2.3.4 51234 22"}}).ssh);
  EXPECT_TRUE(Detect({{"SSH_TTY", "/dev/pts/1"}}).ssh);
}

TEST(TerminalCapabilities, TmuxOverSsh) {
  const TerminalCapabilities caps =
      Detect({{"TMUX", "/tmp/tmux-0/default,1,0"}, {"SSH_CONNECTION", "1.2.3.4 51234 5.6.7.8 22"}});
  EXPECT_TRUE(caps.tmux);
  EXPECT_TRUE(caps.ssh);
  EXPECT_FALSE(caps.screen);
}

TEST(TerminalCapabilities, NestedMultiplexer) {
  // tmux inside GNU screen inside ssh sets all three flags.
  const TerminalCapabilities caps = Detect({{"TMUX", "/tmp/tmux-0/default,1,0"},
                                            {"SSH_CONNECTION", "1.2.3.4 51234 5.6.7.8 22"},
                                            {"STY", "999.pts-0"},
                                            {"TERM", "screen-256color"}});
  EXPECT_TRUE(caps.tmux);
  EXPECT_TRUE(caps.screen);
  EXPECT_TRUE(caps.ssh);
  EXPECT_EQ(caps.color_depth, ColorDepth::k256Color);
}

TEST(TerminalCapabilities, NoColorDisablesOnlyColor) {
  const TerminalCapabilities caps = Detect({{"TERM", "xterm-256color"}, {"NO_COLOR", "1"}});
  EXPECT_EQ(caps.color_depth, ColorDepth::kNone);
  // Other capabilities must be unaffected by NO_COLOR.
  EXPECT_TRUE(caps.unicode);
  EXPECT_TRUE(caps.mouse);
  EXPECT_TRUE(caps.bracketed_paste);
  EXPECT_TRUE(caps.hyperlinks);
  EXPECT_TRUE(caps.osc52);
  EXPECT_TRUE(caps.alternate_screen);
}

TEST(TerminalCapabilities, NoColorWithEmptyValueStillDisablesColor) {
  // NO_COLOR present-but-empty is still "set" per the no-color.org spec.
  const TerminalCapabilities caps = Detect({{"TERM", "xterm-256color"}, {"NO_COLOR", ""}});
  EXPECT_EQ(caps.color_depth, ColorDepth::kNone);
  EXPECT_TRUE(caps.unicode);
}

TEST(TerminalCapabilities, ConflictingColortermAndNoColor) {
  // NO_COLOR sits above COLORTERM, so truecolor is suppressed.
  const TerminalCapabilities caps = Detect({{"COLORTERM", "truecolor"}, {"NO_COLOR", "1"}});
  EXPECT_EQ(caps.color_depth, ColorDepth::kNone);
}

TEST(TerminalCapabilities, ExplicitColorOverrideBeatsNoColor) {
  CapabilityOverrides overrides;
  overrides.color_depth = ColorDepth::kTrueColor;
  const TerminalCapabilities caps = Detect({{"NO_COLOR", "1"}}, overrides);
  EXPECT_EQ(caps.color_depth, ColorDepth::kTrueColor);
}

TEST(TerminalCapabilities, ExplicitEnableOverrideOnUnknownTerminal) {
  CapabilityOverrides overrides;
  overrides.unicode = TriState::kEnable;
  overrides.kitty_graphics = TriState::kEnable;
  const TerminalCapabilities caps = Detect({{"TERM", "weird"}}, overrides);
  EXPECT_TRUE(caps.unicode);
  EXPECT_TRUE(caps.kitty_graphics);
  // Unrelated flags keep their conservative defaults.
  EXPECT_FALSE(caps.mouse);
  EXPECT_FALSE(caps.hyperlinks);
}

TEST(TerminalCapabilities, ExplicitDisableOverrideBeatsTerminal) {
  CapabilityOverrides overrides;
  overrides.osc52 = TriState::kDisable;
  overrides.kitty_graphics = TriState::kDisable;
  const TerminalCapabilities caps = Xterm256WithOverrides(overrides);
  EXPECT_FALSE(caps.osc52);
  EXPECT_FALSE(caps.kitty_graphics);
  EXPECT_TRUE(caps.mouse);
  EXPECT_TRUE(caps.unicode);
}

TEST(TerminalCapabilities, ExplicitDisableBeatsColortermTrueColor) {
  CapabilityOverrides overrides;
  overrides.color_depth = ColorDepth::k16Color;
  const TerminalCapabilities caps =
      Detect({{"COLORTERM", "truecolor"}, {"TERM", "xterm-256color"}}, overrides);
  EXPECT_EQ(caps.color_depth, ColorDepth::k16Color);
}

TEST(TerminalCapabilities, UnknownTermProgramFallsBackToTerm) {
  const TerminalCapabilities caps =
      Detect({{"TERM", "xterm-256color"}, {"TERM_PROGRAM", "com.example.unknown"}});
  // Baseline and identity come from TERM because the program is unrecognized.
  EXPECT_EQ(caps.terminal_identity, "xterm-256color");
  EXPECT_EQ(caps.color_depth, ColorDepth::k256Color);
  EXPECT_TRUE(caps.unicode);
}

TEST(TerminalCapabilities, UnknownTermProgramAloneYieldsItsOwnIdentity) {
  const TerminalCapabilities caps = Detect({{"TERM_PROGRAM", "com.example.unknown"}});
  EXPECT_EQ(caps.terminal_identity, "com.example.unknown");
  EXPECT_FALSE(caps.unicode);
  EXPECT_EQ(caps.color_depth, ColorDepth::kNone);
}

TEST(TerminalCapabilities, MalformedVersionIsTolerated) {
  // A malformed (non-numeric, empty) TERM_PROGRAM_VERSION must never throw or
  // affect detection; it is currently informational only.
  CapabilityOverrides overrides;
  const TerminalCapabilities caps = Detect({{"TERM", "xterm-256color"},
                                            {"TERM_PROGRAM", "kitty"},
                                            {"TERM_PROGRAM_VERSION", "not.a.number"}},
                                           overrides);
  EXPECT_TRUE(caps.kitty_graphics);
  EXPECT_EQ(caps.terminal_identity, "kitty");
  EXPECT_EQ(caps.color_depth, ColorDepth::k256Color);
}

TEST(TerminalCapabilities, EmptyVersionIsTolerated) {
  const TerminalCapabilities caps = Xterm256();
  EXPECT_EQ(caps.color_depth, ColorDepth::k256Color);
  EXPECT_TRUE(caps.unicode);
}

TEST(TerminalCapabilities, IdentityIsStableForSameEnvironment) {
  const std::unordered_map<std::string, std::string> variables = {{"TERM", "xterm-256color"},
                                                                  {"TERM_PROGRAM", "WezTerm"}};
  const MapEnvironment env(variables);
  const TerminalCapabilities first = TerminalDetector{}.Detect(env, {});
  const TerminalCapabilities second = TerminalDetector{}.Detect(env, {});
  EXPECT_EQ(first.terminal_identity, second.terminal_identity);
  EXPECT_EQ(first.terminal_identity, "wezterm");
}

TEST(TerminalCapabilities, OverrideDoesNotChangeIdentity) {
  CapabilityOverrides overrides;
  overrides.unicode = TriState::kDisable;
  const TerminalCapabilities caps = Detect({{"TERM", "xterm-256color"}}, overrides);
  EXPECT_EQ(caps.terminal_identity, "xterm-256color");
}

}  // namespace
}  // namespace terminal
}  // namespace terminal_ui_kit