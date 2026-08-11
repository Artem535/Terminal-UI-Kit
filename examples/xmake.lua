-- Xmake registration for the example applications (PRD section 11.2).
-- Xmake is a secondary, developer-facing frontend; CMake remains
-- authoritative for what actually gets built. Each example here mirrors the
-- corresponding CMake target in examples/CMakeLists.txt.

target("terminal_ui_kit_example_line_number_formatting")
    set_kind("binary")
    add_files("line_number_formatting_example.cpp")
    add_includedirs("include", {public = true})
    add_deps("terminal_ui_kit_core", "terminal_ui_kit_components")
target_end()
