# Task 1 Report — Components CMake wiring, Status, and style bridge

## Scope completed

- Added the eight-value public `terminal_ui_kit::Status` enum.
- Added `to_decorator(const TextStyle&)`, mapping all FTXUI-supported
  non-italic style fields, foreground, and background.
- Converted `TerminalUiKit::Components` from an interface target into a
  compiled FTXUI-backed library.
- Added rendering coverage for foreground, background, combined text flags,
  and an unstyled default `TextStyle`.
- Updated the bare-configure dependency documentation for Components.

## TDD evidence

### RED

Command:

```bash
cmake -S . -B build-debug \
  -DFETCHCONTENT_SOURCE_DIR_FTXUI=/tmp/terminal-ui-kit-ftxui-v5.0.0 \
  -DTERMINAL_UI_KIT_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug \
  && cmake --build build-debug --target terminal_ui_kit_rendering_tests
```

Expected failure observed after adding the test and before production code:

```text
style_bridge_test.cc:1:10: fatal error:
terminal_ui_kit/components/style_bridge.h: No such file or directory
```

### GREEN

Command:

```bash
cmake -S . -B build-debug \
  -DFETCHCONTENT_SOURCE_DIR_FTXUI=/tmp/terminal-ui-kit-ftxui-v5.0.0 \
  -DTERMINAL_UI_KIT_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug \
  && cmake --build build-debug --target terminal_ui_kit_rendering_tests \
  && ctest --test-dir build-debug --output-on-failure -R StyleBridge
```

Output excerpt:

```text
[100%] Built target terminal_ui_kit_rendering_tests
100% tests passed, 0 tests failed out of 4
```

## Final verification

Command:

```bash
cmake --build build-debug && ctest --test-dir build-debug --output-on-failure
```

Output excerpt:

```text
[100%] Built target terminal_ui_kit_unit_tests
100% tests passed, 0 tests failed out of 31
```

`git diff --check` also passed.

## Files changed

- `include/terminal_ui_kit/components/status.h`
- `include/terminal_ui_kit/components/style_bridge.h`
- `src/terminal_ui_kit/components/style_bridge.cc`
- `src/terminal_ui_kit/CMakeLists.txt`
- `cmake/TerminalUiKitOptions.cmake`
- `tests/terminal_ui_kit/rendering/CMakeLists.txt`
- `tests/terminal_ui_kit/rendering/style_bridge_test.cc`

## Self-review

- The enum values match the requested public interface exactly and introduce
  no application-specific types.
- `TextStyle::italic` is explicitly documented as intentionally unmapped,
  because FTXUI does not supply an italic decorator.
- Components now publicly propagates Core and all three required FTXUI
  libraries; its include directory, C++20 requirement, warnings, alias, and
  module registration all match the compiled Core pattern.
- No Xmake files were changed.
- No concerns found.

## Post-review verification

Addressed the Task 1 review findings by removing the dangling design-document
reference, reconciling the Testing comment with Components' unconditional
FTXUI dependency, and applying clang-format to the reviewed source and test.

Command:

```bash
clang-format --dry-run --Werror \
  src/terminal_ui_kit/components/style_bridge.cc \
  tests/terminal_ui_kit/rendering/style_bridge_test.cc \
  && cmake --build build-debug --target terminal_ui_kit_rendering_tests \
  && ctest --test-dir build-debug --output-on-failure -R StyleBridge
```

Output excerpt:

```text
[100%] Built target terminal_ui_kit_rendering_tests
100% tests passed, 0 tests failed out of 4
```
