-- Toast viewer example. Mirrors examples/toast_example/CMakeLists.txt.
-- Links the header-only components target for the public include dirs; as
-- with the other examples, xmake does not compile library sources.
target("terminal_ui_kit_example_toast_example")
    set_kind("binary")
    add_files("main.cc")
    add_includedirs("../../include", {public = true})
    add_deps("terminal_ui_kit_components")