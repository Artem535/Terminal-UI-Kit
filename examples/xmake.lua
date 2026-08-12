-- Xmake registration for example applications (secondary Linux frontend).
-- CMake is authoritative; these targets mirror the CMake example targets for
-- discoverability. FTXUI linkage is not wired through Xmake in this
-- repository (see the top-level xmake.lua), so building these examples
-- through Xmake still requires CMake for FTXUI resolution.

target("terminal_ui_kit_example_line_number_formatting")
    set_kind("binary")
    add_files("line_number_formatting_example.cpp")
    add_deps("terminal_ui_kit_components", "terminal_ui_kit_document")
target_end()
