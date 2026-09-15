# TerminalCapabilities Implementation Plan

> **For agentic workers:** Executed directly; no sub-skill required (small,
> focused module).

**Goal:** Add a deterministic, testable terminal-capability model with
environment detection, conservative defaults, and explicit overrides.

**Architecture:** Pure-data model (`TerminalCapabilities`) + injectable
`EnvironmentProvider` + static detection policy + `CapabilityOverrides`. The
`terminal` module becomes a compiled library independent of FTXUI.

**Tech Stack:** C++20, GoogleTest, CMake (authoritative) + Xmake (secondary
mirror). No optional-feature gating; this module builds unconditionally.

## Global Constraints

- Tests use a map-backed `FakeEnvironment`; never read the real `getenv`.
- Retain owned types only (`std::string`, `std::optional`, enums, `bool`).
- `.clang-format` 100-col, sorted includes; my memory: format with
  `clang-format -i`, and write CMake files via full rewrite (clang-format
  mangles CMakeLists).

### Task 1: Model, environment access, overrides

**Files:** `include/terminal_ui_kit/terminal/{capabilities,environment,overrides}.h`,
`src/terminal_ui_kit/terminal/{capabilities,environment,overrides}.cc`.

- [ ] `ColorDepth` enum + `TerminalCapabilities` (all conservative defaults) +
      `ColorDepthToName`.
- [ ] `EnvironmentProvider` abstract (optional<string> Get) + `SystemEnvironment`.
- [ ] `CapabilityOverrides` (optional per capability) + `ApplyOverrides`.

### Task 2: Detection policy

**Files:** `include/terminal_ui_kit/terminal/detector.h`,
`src/terminal_ui_kit/terminal/detector.cc`.

- [ ] `ParseProgramVersion` (noexcept, defensive).
- [ ] `DetectTerminalCapabilities(env)` per design precedence; color, unicode,
      interactive flags, program-specific image flags, tmux/screen/ssh, identity.
- [ ] `ResolveTerminalCapabilities(env, overrides)` convenience.

### Task 3: Build registration

**Files:** `src/terminal_ui_kit/CMakeLists.txt`, `xmake.lua`.

- [ ] Promote `terminal` to a compiled library (diff pattern); keep
      `TerminalUiKit::Terminal` alias; add to install targets.
- [ ] Xmake: ensure `terminal` module + new example are declared (secondary).

### Task 4: Unit tests

**Files:** `tests/terminal_ui_kit/unit/terminal_capabilities_test.cc`; update
`tests/terminal_ui_kit/unit/CMakeLists.txt`.

- [ ] `FakeEnvironment (EnvironmentProvider)` + `Detect` helper.
- [ ] Tests: empty env, `TERM=dumb`, unknown terminal, 16/256/truecolor
      (COLORTERM and TERM routes, conflicts), Unicode, Kitty, iTerm2 (+version,
      malformed version), Sixel, OSC 52 (+Apple_Terminal), hyperlinks, tmux,
      screen, SSH, tmux-over-SSH, nested, NO_COLOR (non-empty disables color,
      empty does not; unrelated flags intact), unknown TERM_PROGRAM, missing
      variables, override precedence/empty-overrides, stable identity.

### Task 5: Example

**Files:** `examples/terminal_capabilities_example.cpp`, `examples/CMakeLists.txt`,
`examples/xmake.lua` (new), `examples/README.md` (new).

- [ ] FTXUI table of all capabilities + identity.
- [ ] Seven scenarios (real env, `TERM=dumb`, Kitty, iTerm2, tmux-over-SSH,
      unknown, custom override) via number keys; `q` quits; shows synthetic env;
      resize-safe.
- [ ] Register executable in CMake (Components + ftxui pattern) and Xmake.

### Task 6: Documentation

**Files:** `README.md`, `examples/README.md`, docs nav/changelog if applicable.

- [ ] List example + overview in root README.

### Task 7: Verification

- [ ] GCC + Clang strict builds (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion
      -Werror`) of library/tests/example.
- [ ] ASan/UBSan build + tests (Clang, since GCC's libasan is a stub here).
- [ ] `ctest` green; example smoke-run on a few presets.
- [ ] Subagent code review; fix findings in a follow-up commit.

### Task 8: Ship

- [ ] Commits: `Add TerminalCapabilities model`, `Add TerminalCapabilities tests`,
      `Add terminal capabilities example`, plus review fix. Push branch; open PR.