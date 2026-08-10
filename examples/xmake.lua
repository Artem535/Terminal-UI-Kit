-- Xmake example registration (secondary, developer-facing frontend).
-- CMake remains authoritative for what actually gets built and tested; the
-- editor module itself is registered header-only in the root xmake.lua, so the
-- example compiles the editor sources directly for link-ability here.
target("terminal_ui_kit_example_multiline_editor")
    set_kind("binary")
    add_files(
        "multiline_editor/main.cc",
        "../src/terminal_ui_kit/editor/editor_document.cc",
        "../src/terminal_ui_kit/editor/command_history.cc",
        "../src/terminal_ui_kit/editor/multiline_editor.cc")
    add_includedirs("include", "../include", {public = true})
    set_languages("cxx20")
target_end()
