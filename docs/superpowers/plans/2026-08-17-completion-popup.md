# CompletionPopup Implementation Plan

- [x] Inspect repo patterns (components module, VirtualList, tests, CMake/Xmake)
- [x] Write design spec (docs/superpowers/specs/)
- [ ] Header: include/terminal_ui_kit/components/completion_popup.h
- [ ] Source: src/terminal_ui_kit/components/completion_popup.cc
- [ ] Register .cc in src/terminal_ui_kit/CMakeLists.txt
- [ ] Unit tests: tests/terminal_ui_kit/unit/completion_popup_test.cc
- [ ] Rendering tests: tests/terminal_ui_kit/rendering/completion_popup_test.cc
- [ ] Register tests in unit/rendering CMakeLists.txt
- [ ] Example: examples/completion_popup_example.cpp
- [ ] Register example in examples/CMakeLists.txt + examples/xmake.lua
- [ ] examples/README.md documentation
- [ ] GCC debug build + ctest (strict)
- [ ] Clang strict build + ctest
- [ ] ASan/UBSan build + ctest
- [ ] Build + run example in PTY, verify flows
- [ ] Subagent review; fix important findings
- [ ] Commit, push, open PR
