# TerminalCapabilities: Terminal Dimensions Implementation Plan

> **For agentic workers:** Executed directly; no sub-skill required (small,
> focused follow-up to the merged TerminalCapabilities module).

**Issue:** #34 — TerminalCapabilities. The merged module (PR #69) covers color
depth, Unicode, hyperlinks, and the other flags, but the acceptance criteria also
require *relevant terminal dimensions*, which the merged API does not expose.
This plan closes that gap.

**Goal:** Add terminal size (columns/lines in character cells) to the
`TerminalCapabilities` snapshot, detected from the environment and overridable.

**Architecture:** Two new `int` fields on `TerminalCapabilities` (`0` == unknown),
parsed from `COLUMNS`/`LINES` by the existing detection policy, plus two
`std::optional<int>` fields on `CapabilityOverrides`. No new headers, no new
dependencies, no runtime terminal query — the model stays pure-data and
FTXUI-free.

**Tech Stack:** C++20, GoogleTest, CMake (authoritative) + Xmake (secondary).

## Global Constraints

- Tests use the map-backed `FakeEnvironment`; never read the real `getenv`.
- Retain owned types only; no borrowed storage.
- `0` means "unknown" and must degrade safely for malformed input.
- `.clang-format` 100-col, sorted includes; format touched files with
  `clang-format -i`.

### Task 1: Model and overrides

**Files:** `include/terminal_ui_kit/terminal/{capabilities,overrides}.h`,
`src/terminal_ui_kit/terminal/overrides.cc`.

- [x] Add `int columns = 0;` and `int lines = 0;` to `TerminalCapabilities`
      (documented as "0 == unknown").
- [x] Add `std::optional<int> columns;` / `lines;` to `CapabilityOverrides`.
- [x] Apply both in `ApplyOverrides`.

### Task 2: Detection

**Files:** `src/terminal_ui_kit/terminal/detector.cc`.

- [x] `ParseDimension` helper: trim ASCII whitespace, require all-digits,
      reject empty/zero/non-numeric, saturate over-long runs at `INT_MAX`.
- [x] Set `caps.columns` / `caps.lines` from `COLUMNS` / `LINES`.

### Task 3: Unit tests

**Files:** `tests/terminal_ui_kit/unit/terminal_capabilities_test.cc`.

- [x] Empty env => `0` / `0`.
- [x] Present values parsed; whitespace tolerated.
- [x] Empty, non-numeric, zero, negative, over-long => `0` or saturated.
- [x] Overrides win over detection; empty overrides is a no-op.

### Task 4: Example and docs

**Files:** `examples/terminal_capabilities_example.cpp`, `examples/README.md`,
`README.md`, `doc/prd.md`, `docs/modules/ROOT/pages/prd.adoc`.

- [x] Show columns/lines rows in the capability table; add dimensions to a
      scenario and to the custom-override scenario.
- [x] Note dimensions in the example docs and PRD section 33.

### Task 5: Verification

- [x] GCC + Clang strict builds of library/tests/example.
- [x] `ctest` green (539/539).
- [ ] Format touched files; subagent review; fix findings.

### Task 6: Ship

- [ ] Commit `Add terminal dimensions to TerminalCapabilities`; push branch;
      open PR linking #34.
