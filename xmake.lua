-- Xmake is a secondary, developer-facing build frontend for the Linux
-- workflow (PRD section 11.2). CMake remains authoritative for releases,
-- packaging, and CI installation jobs.

set_project("terminal_ui_kit")
set_version("0.1.0")
set_languages("cxx20")
set_warnings("all", "extra")

option("tests")
    set_default(false)
    set_showmenu(true)
    set_description("Build the test suite")
option_end()

option("examples")
    set_default(false)
    set_showmenu(true)
    set_description("Build the example applications")
option_end()

-- Module targets mirror src/terminal_ui_kit/CMakeLists.txt. None has
-- implementation sources yet, so each is a header-only (interface) target.
for _, name in ipairs({"core", "components", "document", "editor", "diff", "markdown", "syntax", "terminal"}) do
    target("terminal_ui_kit_" .. name)
        set_kind("headeronly")
        add_includedirs("include", {public = true})
        add_headerfiles("include/terminal_ui_kit/" .. name .. "/**.h")
    target_end()
end

-- Theme (include/terminal_ui_kit/theme/) has no dedicated target, same as
-- the CMake side (src/terminal_ui_kit/CMakeLists.txt) -- it is exposed
-- through terminal_ui_kit_core.
target("terminal_ui_kit_core")
    add_headerfiles("include/terminal_ui_kit/theme/**.h")
target_end()

if has_config("tests") then
    includes("tests/terminal_ui_kit")
end

if has_config("examples") then
    includes("examples")
end
