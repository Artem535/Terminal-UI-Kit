-- Examples mirror src/terminal_ui_kit/CMakeLists.txt (PRD section 54). Xmake
-- remains a secondary, developer-facing frontend; CMake is authoritative for
-- releases, packaging, and CI. Each example is a standalone executable reached
-- through the project's `examples` option:
--   xmake f --examples
--
-- FTXUI is provided via an Xmake package when `--examples` is enabled.

target("terminal_ui_kit_example_terminal_capabilities")
    set_kind("binary")
    set_languages("cxx20")
    add_includedirs("include", {public = true})
    add_files("terminal_capabilities_example/main.cc")
    add_packages("ftxui", {public = true})
target_end()