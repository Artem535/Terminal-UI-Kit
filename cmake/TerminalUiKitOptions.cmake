# Feature options for Terminal UI Kit.
#
# All options default to OFF except the ones needed to configure and build
# the library itself, so that a bare `cmake -S . -B build` never requires
# network access or optional third-party dependencies.

option(TERMINAL_UI_KIT_BUILD_TESTS "Build the test suite" OFF)
option(TERMINAL_UI_KIT_BUILD_EXAMPLES "Build the example applications" OFF)
option(TERMINAL_UI_KIT_BUILD_BENCHMARKS "Build the performance benchmarks" OFF)
option(TERMINAL_UI_KIT_BUILD_SHARED "Build shared libraries instead of static" OFF)

option(TERMINAL_UI_KIT_ENABLE_MARKDOWN "Enable Markdown rendering support" OFF)
option(TERMINAL_UI_KIT_ENABLE_TREE_SITTER "Enable Tree-sitter syntax highlighting" OFF)
option(TERMINAL_UI_KIT_ENABLE_IMAGES "Enable terminal image rendering" OFF)
option(TERMINAL_UI_KIT_ENABLE_CHAFA "Enable the Chafa image backend" OFF)
option(TERMINAL_UI_KIT_ENABLE_CLIPBOARD "Enable native clipboard integration" OFF)
option(TERMINAL_UI_KIT_ENABLE_OSC52 "Enable OSC 52 clipboard integration" OFF)

option(TERMINAL_UI_KIT_ENABLE_SANITIZERS "Build with ASan/UBSan instrumentation" OFF)
option(TERMINAL_UI_KIT_ENABLE_CLANG_TIDY "Run clang-tidy during the build" OFF)
option(TERMINAL_UI_KIT_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

if(TERMINAL_UI_KIT_BUILD_SHARED)
  set(BUILD_SHARED_LIBS ON)
endif()
