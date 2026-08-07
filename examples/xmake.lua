-- Xmake mirrors the example targets for discoverability (PRD section 11.2);
-- CMake remains authoritative for building and testing them.

target("terminal_ui_kit_example_searchable_text_view")
    set_kind("binary")
    add_files("searchable_text_view_example/main.cc")
    add_includedirs("include", "../include", {public = true})
    add_deps("terminal_ui_kit_components", "terminal_ui_kit_search")
target_end()