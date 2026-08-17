-- Xmake frontend for example applications (secondary; CMake is authoritative).
--
-- This file is only loaded when the `examples` option is enabled:
--   xmake f --examples=y
-- Under CMake (the authoritative frontend) examples are driven entirely by
-- examples/CMakeLists.txt.
--
-- The completion-popup example reproduces the CMake dependency shape: Xmake
-- does not compile `src/terminal_ui_kit` for the library mirrors (they stay
-- header-only), so the example reaches for the same .cc files the Components /
-- Core / Document CMake targets compile, plus FTXUI from the local install
-- prefix, matching the environment the library is laid out for here.

set_languages("cxx20")

local prefix = os.getenv("HOME") .. "/.local"

target("terminal_ui_kit_example_completion_popup")
    set_kind("binary")
    add_includedirs("include", {public = true})
    add_includedirs("include")
    add_includedirs(prefix .. "/include")
    add_linkdirs(prefix .. "/lib64")
    add_links("ftxui-component", "ftxui-dom", "ftxui-screen", "pthread")
    add_files("completion_popup_example.cpp")
    add_files("../src/terminal_ui_kit/components/completion_popup.cc")
    add_files("../src/terminal_ui_kit/components/style_bridge.cc")
    add_files("../src/terminal_ui_kit/components/key_hint_bar.cc")
    add_files("../src/terminal_ui_kit/core/*.cc")
    add_files("../src/terminal_ui_kit/theme/*.cc")
    add_files("../src/terminal_ui_kit/document/*.cc")
    set_warnings("all", "extra", "pedantic", "shadow", "conversion", "error")
target_end()
