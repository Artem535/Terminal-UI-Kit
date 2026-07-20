# Dependency resolution helpers for Terminal UI Kit.
#
# Each dependency is resolved lazily through a `terminal_ui_kit_require_*()`
# function instead of being fetched unconditionally at the top of the
# project. This keeps a bare `cmake -S . -B build` of the skeleton free of
# network access; only the subdirectories that actually compile against a
# dependency call the matching function (see PRD section 8.5, "Optional
# Dependencies", and section 48, "Dependencies").
#
# find_package() is always tried first so a toolchain file (e.g. vcpkg's)
# can satisfy the dependency without touching the network at all.

include(FetchContent)

function(terminal_ui_kit_require_ftxui)
  find_package(ftxui CONFIG QUIET)
  if(TARGET ftxui::screen)
    return()
  endif()

  message(STATUS "FTXUI not found via find_package(); fetching with FetchContent")
  FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
    GIT_TAG v5.0.0
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(ftxui)
endfunction()

function(terminal_ui_kit_require_gtest)
  find_package(GTest CONFIG QUIET)
  if(TARGET GTest::gtest)
    return()
  endif()

  message(STATUS "GoogleTest not found via find_package(); fetching with FetchContent")
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(googletest)
endfunction()

function(terminal_ui_kit_require_benchmark)
  find_package(benchmark CONFIG QUIET)
  if(TARGET benchmark::benchmark)
    return()
  endif()

  message(STATUS "Google Benchmark not found via find_package(); fetching with FetchContent")
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.1
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(benchmark)
endfunction()
