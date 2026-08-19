-- Xmake mirror of examples/CMakeLists.txt (PRD section 11.2 -- Xmake is a
-- secondary, developer-facing frontend; CMake remains authoritative for what
-- actually gets built). Loaded only when xmake is run with --examples.

target("terminal_ui_kit_example_unified_diff_view")
    set_kind("binary")
    set_languages("cxx20")
    add_files("unified_diff_view_example.cpp")
    add_includedirs("include", {public = true})
    add_deps("terminal_ui_kit_components", "terminal_ui_kit_diff")
    -- FTXUI is provided by the system/package manager; the example uses only
    -- the public TerminalUiKit API plus FTXUI's own component/screen headers.
    add_includedirs("/usr/include", "/usr/local/include",
                    os.getenv("HOME") .. "/.local/include", {public = true})
