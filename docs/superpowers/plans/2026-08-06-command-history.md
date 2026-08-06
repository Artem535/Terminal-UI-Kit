# CommandHistory — implementation plan

- [x] Inspect repo (AGENTS.md, CMake/Xmake, neighbouring non-FTXUI models, examples)
- [x] Write plan + short design spec
- [x] Implement `include/terminal_ui_kit/command/command_history.h` (header-only, no FTXUI)
  - `CommandHistory` with bounded storage, `Previous()/Next()`, `Search()/SearchPrefix()`, `Clear()/Size()/Capacity()`
  - `CommandHistoryPersistence` adapter interface (`Save`/`Clear`)
  - `CommandSensitivePolicy` interface + `NeverSensitivePolicy` + `PrefixSensitivePolicy`
- [x] Wire build: CMake `command` INTERFACE module + unit-test target + example target; Xmake module + example
- [x] Unit tests: `tests/terminal_ui_kit/unit/command_history_test.cc`
- [x] Example: `examples/command_history_example.cpp` (FTXUI interactive) + `examples/README.md`
- [x] Build + test with GCC and Clang (`-Werror`)
- [x] ASan + UBSan run
- [x] Run example and verify main flows
- [x] Subagent review + fix important findings
- [x] Commit, push, open PR