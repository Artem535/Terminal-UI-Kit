-- Example applications (PRD section 54). CMake remains the authoritative
-- frontend for examples; this Xmake file registers the example targets so the
-- `xmake f --examples=true` option discovers them, mirroring how the module
-- targets are mirrored (header-only) in the root xmake.lua.
--
-- Note: like the root library targets, these are developer-facing mirrors.
-- FTXUI linkage is provided by CMake (find_package(ftxui)); an Xmake build that
-- actually links the client runner needs FTXUI configured via add_requires.

target("terminal_ui_kit_example_line_number_formatting")
    set_kind("binary")
    set_languages("cxx20")
    add_files("line_number_formatting_example.cpp")
    add_includedirs("../include", {public = true})
    add_deps("terminal_ui_kit_components", "terminal_ui_kit_document")
target_end()