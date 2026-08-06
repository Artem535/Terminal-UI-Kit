-- Xmake is a secondary, developer-facing frontend (PRD section 11.2). It
-- fetches no external dependencies, so the FTXUI-backed example is only built
-- here when the FTXUI headers and libraries are already available on the host.
-- CMake remains authoritative and builds the same target for all users.

local detect = import("lib.detect")

if detect.find_header("ftxui/screen/screen.hpp") and
    (detect.find_library("ftxui::screen") or detect.find_library("ftxui-screen") or detect.find_library("screen")) then
    target("terminal_ui_kit_example_command_history")
        set_kind("binary")
        set_languages("cxx20")
        add_includedirs("../include", {public = true})
        add_files("command_history_example.cpp")
        add_cxflags("-Wall", "-Wextra", "-Wpedantic", "-Wshadow", "-Wconversion")
    target_end()
end