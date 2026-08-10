-- Xmake mirror of the example targets. CMake remains authoritative for
-- actually building and running examples; this file registers the same
-- top-level example targets so `xmake` can discover them. The library modules
-- are header-only mirrors (see xmake.lua at the repo root), so these targets
-- are listed for parity and are exercised through CMake for real linking.

if not has_config("examples") then
    return
end

target("terminal_ui_kit_example_unified_diff_view")
    set_kind("binary")
    set_languages("cxx20")
    add_files("unified_diff_view_example.cpp")
    add_includedirs("include", {public = true})
    add_deps("terminal_ui_kit_components", "terminal_ui_kit_diff")
    add_links("ftxui-component", "ftxui-dom", "ftxui-screen")
target_end()
