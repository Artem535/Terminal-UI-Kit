-- Xmake registration for the example applications (PRD section 11.2).
-- Xmake is a secondary, developer-facing frontend; CMake remains
-- authoritative for what actually gets built. Each example here mirrors the
-- corresponding CMake target in examples/CMakeLists.txt.
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

target("terminal_ui_kit_example_completion_popup")
    set_kind("binary")
    add_includedirs("include", {public = true})
    add_includedirs("include")
    add_includedirs(prefix .. "/include")
    add_linkdirs(prefix .. "/lib64")
    add_links("ftxui-component", "ftxui-dom", "ftxui-screen", "pthread")
    add_files("completion_popup_example.cpp")
    add_files("../src/terminal_ui_kit/components/completion_popup.cc")
    add_files("../src/terminal_ui_kit/components/style_bridge.cc")
    add_files("../src/terminal_ui_kit/components/key_hint_bar.cc")
    add_files("../src/terminal_ui_kit/core/*.cc")
    add_files("../src/terminal_ui_kit/theme/*.cc")
    add_files("../src/terminal_ui_kit/document/*.cc")
    set_warnings("all", "extra", "pedantic", "shadow", "conversion", "error")
target_end()
target("terminal_ui_kit_example_searchable_text_view")
    set_kind("binary")
    add_files("searchable_text_view_example/main.cc")
    add_includedirs("include", "../include", {public = true})
    add_deps("terminal_ui_kit_components", "terminal_ui_kit_search")
target_end()

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

target("terminal_ui_kit_example_line_number_formatting")
    set_kind("binary")
    add_files("line_number_formatting_example.cpp")
    add_includedirs("include", {public = true})
    add_deps("terminal_ui_kit_core", "terminal_ui_kit_components")
target_end()

target("terminal_ui_kit_example_unified_diff_view")
    set_kind("binary")
    set_languages("cxx20")
    add_files("unified_diff_view_example.cpp")
    add_includedirs("include", {public = true})
    add_deps("terminal_ui_kit_components", "terminal_ui_kit_diff")
    add_links("ftxui-component", "ftxui-dom", "ftxui-screen")
target_end()

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

-- Xmake is a secondary, developer-facing build frontend for the Linux
-- workflow (PRD section 11.2). CMake remains authoritative for releases,
-- packaging, and CI installation jobs: library targets in the root xmake.lua
-- are header-only mirrors that do not compile the implementation .cc files.
--
-- The TranscriptView example is registered here for target discoverability
-- alongside its CMake counterpart (`terminal_ui_kit_example_transcript_view`).
-- Because Xmake does not compile the Components implementation, this target is
-- a header-only mirror of the example source + component headers rather than a
-- buildable binary; build and run it via CMake (see examples/README.md).
target("terminal_ui_kit_example_transcript_view")
    set_kind("headeronly")
    add_headerfiles("transcript_view_example/transcript_view_example.cpp",
                    "include/terminal_ui_kit/components/transcript.h",
                    "include/terminal_ui_kit/components/transcript_model.h")
target_end()