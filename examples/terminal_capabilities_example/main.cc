// Example: TerminalCapabilities
//
// An interactive tour of the terminal-capability model. It renders the
// detected capabilities (color depth, Unicode, mouse, bracketed paste,
// hyperlinks, OSC 52, kitty/sixel/iTerm images, alternate screen, tmux,
// screen, ssh, identity) for a selectable environment preset.
//
// Detection never probes the live terminal: each preset is a deterministic
// MapEnvironment (except "real", which reads the actual process environment
// through ProcessEnvironment). The active synthetic environment and the
// available controls are always on screen.
//
// Keys:
//   Left / Right  switch environment preset
//   q             quit

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/terminal/environment.h"
#include "terminal_ui_kit/terminal/terminal_capabilities.h"
#include "terminal_ui_kit/terminal/terminal_detector.h"

using ftxui::Element;
using terminal_ui_kit::terminal::CapabilityOverrides;
using terminal_ui_kit::terminal::ColorDepth;
using terminal_ui_kit::terminal::EnvironmentProvider;
using terminal_ui_kit::terminal::MapEnvironment;
using terminal_ui_kit::terminal::ProcessEnvironment;
using terminal_ui_kit::terminal::TerminalCapabilities;
using terminal_ui_kit::terminal::TerminalDetector;
using terminal_ui_kit::terminal::TriState;

namespace {

std::string ColorDepthName(ColorDepth depth) {
  switch (depth) {
    case ColorDepth::kNone:
      return "none";
    case ColorDepth::k16Color:
      return "16";
    case ColorDepth::k256Color:
      return "256";
    case ColorDepth::kTrueColor:
      return "truecolor";
  }
  return "?";
}

std::unique_ptr<EnvironmentProvider> MakeEnv(
    const std::vector<std::pair<std::string, std::string>>& variables) {
  std::unordered_map<std::string, std::string> map;
  for (const auto& [key, value] : variables) {
    map[key] = value;
  }
  return std::make_unique<MapEnvironment>(std::move(map));
}

// One selectable environment preset.
struct Preset {
  std::string name;
  std::unique_ptr<EnvironmentProvider> provider = nullptr;
  CapabilityOverrides overrides = {};
  // Human-readable description of the synthetic environment (empty for the
  // real-environment preset, whose values are the live ones).
  std::vector<std::pair<std::string, std::string>> synthetic_env = {};
};

std::vector<Preset> MakePresets() {
  std::vector<Preset> presets;

  {
    Preset preset{"real environment"};
    preset.provider = std::make_unique<ProcessEnvironment>();
    presets.push_back(std::move(preset));
  }
  {
    Preset preset{"TERM=dumb"};
    preset.provider = MakeEnv({{"TERM", "dumb"}});
    preset.synthetic_env = {{"TERM", "dumb"}};
    presets.push_back(std::move(preset));
  }
  {
    Preset preset{"kitty"};
    preset.provider = MakeEnv(
        {{"TERM", "xterm-256color"}, {"COLORTERM", "truecolor"}, {"TERM_PROGRAM", "kitty"}});
    preset.synthetic_env = {
        {"TERM", "xterm-256color"}, {"COLORTERM", "truecolor"}, {"TERM_PROGRAM", "kitty"}};
    presets.push_back(std::move(preset));
  }
  {
    Preset preset{"iTerm2"};
    preset.provider = MakeEnv(
        {{"TERM", "xterm-256color"}, {"COLORTERM", "truecolor"}, {"TERM_PROGRAM", "iTerm.app"}});
    preset.synthetic_env = {
        {"TERM", "xterm-256color"}, {"COLORTERM", "truecolor"}, {"TERM_PROGRAM", "iTerm.app"}};
    presets.push_back(std::move(preset));
  }
  {
    Preset preset{"tmux over SSH"};
    preset.provider = MakeEnv({{"TERM", "tmux-256color"},
                               {"TMUX", "/tmp/tmux-0/default,1,0"},
                               {"SSH_CONNECTION", "1.2.3.4 51234 5.6.7.8 22"}});
    preset.synthetic_env = {{"TERM", "tmux-256color"},
                            {"TMUX", "/tmp/tmux-0/default,1,0"},
                            {"SSH_CONNECTION", "1.2.3.4 51234 5.6.7.8 22"}};
    presets.push_back(std::move(preset));
  }
  {
    Preset preset{"unknown terminal"};
    preset.provider = MakeEnv({{"TERM", "weird"}});
    preset.synthetic_env = {{"TERM", "weird"}};
    presets.push_back(std::move(preset));
  }
  {
    Preset preset{"custom override"};
    preset.provider = MakeEnv({{"TERM", "xterm-256color"}});
    preset.overrides.unicode = TriState::kEnable;
    preset.overrides.kitty_graphics = TriState::kEnable;
    preset.overrides.osc52 = TriState::kDisable;
    preset.overrides.color_depth = ColorDepth::kTrueColor;
    preset.synthetic_env = {{"TERM", "xterm-256color"},
                            {"override", "unicode/kitty on, OSC 52 off, truecolor"}};
    presets.push_back(std::move(preset));
  }

  return presets;
}

Element BoolRow(const std::string& label, bool value) {
  return ftxui::hbox({ftxui::text(label) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 24) | ftxui::dim,
                      ftxui::text(value ? "yes" : "no") | ftxui::bold});
}

Element CapabilityTable(const TerminalCapabilities& caps) {
  ftxui::Elements green_rows;
  std::vector<std::pair<std::string, bool>> flags = {
      {"unicode", caps.unicode},
      {"mouse", caps.mouse},
      {"bracketed paste", caps.bracketed_paste},
      {"hyperlinks", caps.hyperlinks},
      {"kitty graphics", caps.kitty_graphics},
      {"sixel", caps.sixel},
      {"iTerm images", caps.iterm_images},
      {"OSC 52", caps.osc52},
      {"alternate screen", caps.alternate_screen},
      {"tmux", caps.tmux},
      {"screen", caps.screen},
      {"ssh", caps.ssh},
  };
  for (const auto& [label, value] : flags) {
    green_rows.push_back(BoolRow(label, value));
  }
  return ftxui::vbox({
      ftxui::hbox(
          {ftxui::text("color depth") | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 24) | ftxui::dim,
           ftxui::text(ColorDepthName(caps.color_depth)) | ftxui::bold}),
      ftxui::vbox(std::move(green_rows)),
      ftxui::separator(),
      ftxui::hbox({ftxui::text("terminal identity") | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 24) |
                       ftxui::dim,
                   ftxui::text(caps.terminal_identity) | ftxui::bold}),
  });
}

Element EnvironmentTable(const std::vector<std::pair<std::string, std::string>>& env) {
  ftxui::Elements rows;
  for (const auto& [key, value] : env) {
    rows.push_back(ftxui::text(key + "=" + value) | ftxui::color(ftxui::Color::GrayLight));
  }
  if (rows.empty()) {
    rows.push_back(ftxui::text("(live process environment)") | ftxui::dim);
  }
  return ftxui::vbox(std::move(rows));
}

}  // namespace

int main() {
  const TerminalDetector detector;
  std::vector<Preset> presets = MakePresets();
  int selected = 0;

  auto screen = ftxui::ScreenInteractive::Fullscreen();
  auto component = ftxui::Renderer([&] {
    const Preset& preset = presets[static_cast<size_t>(selected)];
    const TerminalCapabilities caps = detector.Detect(*preset.provider, preset.overrides);

    ftxui::Elements env_lines;
    for (int i = 0; i < static_cast<int>(presets.size()); ++i) {
      const std::string marker = (i == selected) ? "> " : "  ";
      env_lines.push_back(ftxui::text(marker + presets[static_cast<size_t>(i)].name));
    }

    return ftxui::vbox({
               ftxui::text("Terminal UI Kit - Terminal Capabilities") | ftxui::bold,
               ftxui::text("Deterministic, environment-based capability detection.") | ftxui::dim,
               ftxui::separator(),
               ftxui::hbox({
                   ftxui::vbox(
                       {ftxui::text("Preset") | ftxui::bold, ftxui::vbox(std::move(env_lines))}),
                   ftxui::separator(),
                   ftxui::vbox({ftxui::text("Active environment") | ftxui::bold,
                                EnvironmentTable(preset.synthetic_env)}),
                   ftxui::separator(),
                   ftxui::vbox({ftxui::text("Capabilities") | ftxui::bold, CapabilityTable(caps)}),
               }),
               ftxui::separator(),
               ftxui::text("Controls: Left/Right switch preset - q quit") | ftxui::dim,
           }) |
           ftxui::border;
  });

  component |= ftxui::CatchEvent([&](const ftxui::Event& event) {
    if (event == ftxui::Event::ArrowLeft) {
      selected =
          (selected - 1 + static_cast<int>(presets.size())) % static_cast<int>(presets.size());
      return true;
    }
    if (event == ftxui::Event::ArrowRight) {
      selected = (selected + 1) % static_cast<int>(presets.size());
      return true;
    }
    if (event == ftxui::Event::Character('q')) {
      screen.ExitLoopClosure()();
      return true;
    }
    return false;
  });

  screen.Loop(component);
}