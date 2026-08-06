-- Example applications (PRD section 54). CMake is authoritative for actually
-- building and running examples; Xmake mirrors the primary example targets
-- for discoverability only, in the same spirit as the header-only module
-- targets in the root xmake.lua. This file is only processed when xmake is
-- invoked with `--examples=y`.

target("terminal_ui_kit_example_line_number_formatting")
    set_kind("binary")
    add_files("line_number_formatting_example/main.cc")
    add_includedirs("include", {public = true})
    add_deps("terminal_ui_kit_components", "terminal_ui_kit_document")
target_end()
