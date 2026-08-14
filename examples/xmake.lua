-- Xmake frontend for example applications (secondary; CMake is authoritative).
--
-- This file is only loaded when the `examples` option is enabled:
--   xmake f --examples=y
-- Under CMake (the authoritative frontend) examples are driven entirely by
-- examples/CMakeLists.txt.
--
-- The terminal-capabilities example reproduces the CMake dependency shape:
-- it links the terminal module sources directly (Xmake does not compile
-- `src/terminal_ui_kit/terminal` for the library mirror, so the example
-- reaches for the same .cc files) and FTXUI from the local install prefix,
-- matching the environment the library is laid out for here. It depends on the
-- header-only `terminal_ui_kit_core` mirror too, so any Core header dependency
-- the terminal sources gain is satisfied without extra wiring.

set_languages("cxx20")

local prefix = os.getenv("HOME") .. "/.local"

target("terminal_ui_kit_example_terminal_capabilities")
    set_kind("binary")
    add_deps("terminal_ui_kit_core")
    add_includedirs("include", "../../include", {public = true})
    add_includedirs(prefix .. "/include")
    add_linkdirs(prefix .. "/lib64")
    add_links("ftxui-component", "ftxui-dom", "ftxui-screen")
    add_files("terminal_capabilities_example.cpp")
    add_files("../src/terminal_ui_kit/terminal/*.cc")
    set_warnings("all", "extra", "pedantic", "shadow", "conversion", "error")
target_end()