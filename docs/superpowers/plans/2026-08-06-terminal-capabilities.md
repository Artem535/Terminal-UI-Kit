# TerminalCapabilities Implementation Plan

**Goal:** Add a deterministic, testable terminal-capability model with environment
detection, conservative defaults, and explicit enable/disable overrides, plus a
standalone example that switches among presets.

**Architecture:** Four separated concerns, each with its own header:
capability data (`terminal_capabilities.h`), environment access
(`environment.h`), detection policy + user overrides (`terminal_detector.h`).
Detection is a pure function of an injectable `EnvironmentProvider`, so tests
never read the real process environment.

**Tech Stack:** C++20, FTXUI (example only), GoogleTest, CMake, Xmake.

## Global Constraints

- Tests must not depend on the real process environment — always pass an
  explicit `MapEnvironment`/fake provider.
- Detection is conservative: unknown signals produce the lowest-risk defaults.
- Overrides (explicit enable/disable) win over every environment signal,
  including `NO_COLOR`. `NO_COLOR` only affects color depth, never unrelated
  capabilities.
- The `terminal` CMake target becomes a compiled library (currently INTERFACE);
  Xmake keeps mirroring the same headers, which its existing `terminal` glob
  already does.

### Task 1: Model + environment access

**Files:** create `terminal_capabilities.h`, `environment.h`.

- [ ] `ColorDepth` enum (`kNone`, `k16Color`, `k256Color`, `kTrueColor`).
- [ ] `TerminalCapabilities` data struct matching the task scope.
- [ ] `EnvironmentProvider` interface, `ProcessEnvironment` (reads `std::getenv`),
  `MapEnvironment` (deterministic map-backed provider for tests/example).

### Task 2: Detection policy + overrides

**Files:** create `terminal_detector.h`, `terminal_detector.cc`.

- [ ] `TriState` (`kDefault`/`kEnable`/`kDisable`) and `CapabilityOverrides`
  (one member per boolean capability + optional color depth).
- [ ] `TerminalDetector::Detect(env, overrides)` implementing the precedence
  and preset table (documented in the header).

### Task 3: Tests

**Files:** create `tests/terminal_ui_kit/unit/terminal_capabilities_test.cc`,
modify unit `CMakeLists.txt`.

- [ ] Cover every required environment plus conflicting vars, `NO_COLOR`
  isolation, unknown `TERM_PROGRAM`, override precedence, malformed versions,
  nested tmux/SSH, stable identity.
- [ ] Route through `MapEnvironment`; never read real env in tests.

### Task 4: Example + docs

**Files:** create `examples/terminal_capabilities_example/main.cc` and
`CMakeLists.txt`, `examples/README.md`; modify `examples/CMakeLists.txt`,
root `xmake.lua`/`examples/xmake.lua`.

- [ ] Interactive FTXUI app switching among real env / dumb / kitty / iTerm2 /
  tmux-over-SSH / unknown / custom override; `q` exits; works across resize.

### Task 5: Verification

- [ ] GCC strict build (`-Werror`), Clang strict build, ASan/UBSan run.
- [ ] Run example, verify flows, subagent review, commit/push/PR.