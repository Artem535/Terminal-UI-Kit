// Example: TerminalCapabilities
//
// Interactive FTXUI demo of the terminal-capability model in
// terminal_ui_kit/terminal. It renders a capability table (color depth,
// unicode, mouse, bracketed paste, hyperlinks, OSC 52, kitty graphics, sixel,
// iTerm images, alternate screen, tmux, screen, SSH, and terminal identity)
// for a selectable scenario.
//
// The example uses only the public Terminal module API. It does not detect the
// running terminal at runtime by querying it; instead it computes capabilities
// from an environment provider, which is switched between:
//   1. the real process environment (SystemEnvironment);
//   2..7. deterministic synthetic environments (a snapshot provider) including
//   a custom CapabilityOverrides scenario.
//
// Controls:
//   [1..7]  switch to scenario N (see list on screen; "1" = real environment)
//   [q]     quit
//
// The layout is a plain FTXUI tree, so it reflows automatically on resize.
//
// Usage:
//   terminal_ui_kit_example_terminal_capabilities

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include "terminal_ui_kit/terminal/capabilities.h"
#include "terminal_ui_kit/terminal/detector.h"
#include "terminal_ui_kit/terminal/environment.h"
#include "terminal_ui_kit/terminal/overrides.h"

namespace {

using terminal_ui_kit::terminal::CapabilityOverrides;
using terminal_ui_kit::terminal::ColorDepth;
using terminal_ui_kit::terminal::ColorDepthToName;
using terminal_ui_kit::terminal::EnvironmentProvider;
using terminal_ui_kit::terminal::ResolveTerminalCapabilities;
using terminal_ui_kit::terminal::SystemEnvironment;
using terminal_ui_kit::terminal::TerminalCapabilities;

// EnvironmentProvider backed by an in-memory snapshot, so a scenario can be
// fully deterministic and independent of the real process environment.
class SnapshotEnvironment : public EnvironmentProvider {
 public:
  explicit SnapshotEnvironment(std::map<std::string, std::string, std::less<>> vars)
      : vars_(std::move(vars)) {}

  std::optional<std::string> Get(const std::string& name) const override {
    const auto it = vars_.find(name);
    if (it == vars_.end()) return std::nullopt;
    return it->second;
  }

 private:
  std::map<std::string, std::string, std::less<>> vars_;
};

// One selectable scenario. `is_live` means "read the real process environment";
// otherwise `vars` supplies the synthetic environment.
struct Scenario {
  std::string name;
  bool is_live = false;
  std::map<std::string, std::string, std::less<>> vars;
  CapabilityOverrides overrides;
};

// The custom-override scenario demonstrates explicit user overrides winning
// over whatever the (synthetic) environment would imply.
CapabilityOverrides CustomOverrides() {
  CapabilityOverrides overrides;
  overrides.color_depth = ColorDepth::kTrueColor;
  overrides.unicode = false;  // explicit disable, beats the detected default
  overrides.sixel = true;     // explicit enable on an otherwise-unset program
  overrides.osc52 = false;    // explicit disable
  return overrides;
}

const std::vector<Scenario>& Scenarios() {
  static const std::vector<Scenario> kScenarios = {
      {"Real environment", true, {}, {}},
      {"TERM=dumb", false, {{"TERM", "dumb"}}, {}},
      {"Kitty",
       false,
       {{"TERM", "xterm-kitty"}, {"TERM_PROGRAM", "kitty"}, {"COLORTERM", "truecolor"}},
       {}},
      {"iTerm2",
       false,
       {{"TERM", "xterm-256color"},
        {"TERM_PROGRAM", "iTerm.app"},
        {"TERM_PROGRAM_VERSION", "3.4.19"},
        {"COLORTERM", "truecolor"}},
       {}},
      {"tmux over SSH",
       false,
       {{"TERM", "tmux-256color"},
        {"TMUX", "/tmp/tmux-1000/default,12345,0"},
        {"SSH_CONNECTION", "192.168.1.10 54321 192.168.1.1 22"}},
       {}},
      {"Unknown terminal", false, {{"TERM", "unknownxyz"}}, {}},
      {"Custom override",
       false,
       {{"TERM", "xterm"}, {"COLORTERM", "truecolor"}},
       CustomOverrides()},
  };
  return kScenarios;
}

std::unique_ptr<EnvironmentProvider> BuildProvider(const Scenario& scenario) {
  if (scenario.is_live) return std::make_unique<SystemEnvironment>();
  return std::make_unique<SnapshotEnvironment>(scenario.vars);
}

std::string YesNo(bool value) { return value ? "yes" : "no"; }

// A single table row: left-aligned label, then the value.
ftxui::Element Row(const std::string& label, const std::string& value) {
  return ftxui::hbox({
      ftxui::text(label) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 26) | ftxui::dim,
      ftxui::text(value),
  });
}

// Renders the capability table for the currently selected scenario.
ftxui::Element CapabilityTable(const TerminalCapabilities& caps) {
  ftxui::Elements rows;
  rows.push_back(Row("Color depth", ColorDepthToName(caps.color_depth)));
  rows.push_back(Row("Unicode", YesNo(caps.unicode)));
  rows.push_back(Row("Mouse", YesNo(caps.mouse)));
  rows.push_back(Row("Bracketed paste", YesNo(caps.bracketed_paste)));
  rows.push_back(Row("Hyperlinks", YesNo(caps.hyperlinks)));
  rows.push_back(Row("OSC 52", YesNo(caps.osc52)));
  rows.push_back(Row("Kitty graphics", YesNo(caps.kitty_graphics)));
  rows.push_back(Row("Sixel", YesNo(caps.sixel)));
  rows.push_back(Row("iTerm images", YesNo(caps.iterm_images)));
  rows.push_back(Row("Alternate screen", YesNo(caps.alternate_screen)));
  rows.push_back(Row("tmux", YesNo(caps.tmux)));
  rows.push_back(Row("screen", YesNo(caps.screen)));
  rows.push_back(Row("SSH", YesNo(caps.ssh)));
  rows.push_back(
      Row("Terminal identity", caps.terminal_identity.empty() ? "(none)" : caps.terminal_identity));
  return ftxui::vbox(std::move(rows));
}

// Renders the active synthetic environment (or a "(live)" marker).
ftxui::Element EnvironmentSummary(const Scenario& scenario) {
  if (scenario.is_live) {
    return ftxui::text("Environment: real process environment (live)") | ftxui::dim;
  }
  std::string summary;
  for (const auto& [name, value] : scenario.vars) {
    if (!summary.empty()) summary += "  ";
    summary += name + "=" + value;
  }
  return ftxui::text("Environment: " + (summary.empty() ? "(empty)" : summary)) | ftxui::dim;
}

ftxui::Element OverridesSummary(const CapabilityOverrides& overrides) {
  ftxui::Elements lines;
  auto push = [&](const std::string& name, const std::optional<bool>& value) {
    if (value.has_value()) lines.push_back(Row("  override " + name, YesNo(*value)));
  };
  if (overrides.color_depth.has_value()) {
    lines.push_back(Row("  override color_depth", ColorDepthToName(*overrides.color_depth)));
  }
  push("unicode", overrides.unicode);
  push("mouse", overrides.mouse);
  push("bracketed_paste", overrides.bracketed_paste);
  push("hyperlinks", overrides.hyperlinks);
  push("kitty_graphics", overrides.kitty_graphics);
  push("sixel", overrides.sixel);
  push("iterm_images", overrides.iterm_images);
  push("osc52", overrides.osc52);
  push("alternate_screen", overrides.alternate_screen);
  push("tmux", overrides.tmux);
  push("screen", overrides.screen);
  push("ssh", overrides.ssh);
  if (overrides.terminal_identity.has_value()) {
    lines.push_back(Row("  override terminal_identity", *overrides.terminal_identity));
  }
  if (lines.empty()) {
    return ftxui::text("Overrides: none") | ftxui::dim;
  }
  lines.insert(lines.begin(), ftxui::text("Overrides:") | ftxui::dim);
  return ftxui::vbox(std::move(lines));
}

}  // namespace

int main() {
  auto screen = ftxui::ScreenInteractive::Fullscreen();
  std::size_t current = 0;  // scenario index

  auto renderer = ftxui::Renderer([&] {
    const Scenario& scenario = Scenarios()[current];
    const std::unique_ptr<EnvironmentProvider> provider = BuildProvider(scenario);
    const TerminalCapabilities caps = ResolveTerminalCapabilities(*provider, scenario.overrides);

    ftxui::Elements scenario_list;
    for (std::size_t i = 0; i < Scenarios().size(); ++i) {
      const std::string marker = (i == current) ? " >" : "  ";
      scenario_list.push_back(
          ftxui::text(marker + " [" + std::to_string(i + 1) + "] " + Scenarios()[i].name));
    }

    return ftxui::vbox({
               ftxui::text("Terminal UI Kit - Terminal Capabilities") | ftxui::bold,
               ftxui::text("Detect terminal capabilities from the environment.") | ftxui::dim,
               ftxui::separator(),
               CapabilityTable(caps),
               ftxui::separator(),
               EnvironmentSummary(scenario),
               OverridesSummary(scenario.overrides),
               ftxui::separator(),
               ftxui::vbox(std::move(scenario_list)) | ftxui::border |
                   ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 60),
               ftxui::separator(),
               ftxui::text("Controls") | ftxui::bold,
               ftxui::text("[1..7] choose scenario   [q] quit"),
           }) |
           ftxui::border;
  });

  renderer |= ftxui::CatchEvent([&](ftxui::Event event) {
    if (!event.is_character()) return false;
    const std::string character = event.character();
    if (character.size() != 1) return false;
    const char key = character[0];
    if (key >= '1' && key <= '7') {
      current = static_cast<std::size_t>(key - '1');
      return true;
    }
    if (key == 'q') {
      screen.ExitLoopClosure()();
      return true;
    }
    return false;
  });

  screen.Loop(renderer);
  return 0;
}