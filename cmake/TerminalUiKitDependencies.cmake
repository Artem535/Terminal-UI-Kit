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
  # Ensure FTXUI is compiled with -fPIC so it can be linked into shared
  # libraries (terminal_ui_kit_markdown when BUILD_SHARED_LIBS=ON).
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
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

function(terminal_ui_kit_require_cmark_gfm)
  find_package(cmark-gfm CONFIG QUIET)
  if(TARGET cmark-gfm::cmark-gfm OR TARGET cmark::cmark)
    return()
  endif()

  message(STATUS "cmark-gfm not found via find_package(); fetching with FetchContent")
  set(CMARK_TESTS OFF CACHE BOOL "" FORCE)
  set(CMARK_SHARED OFF CACHE BOOL "" FORCE)
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE BOOL "" FORCE)
  FetchContent_Declare(
    cmark-gfm
    GIT_REPOSITORY https://github.com/github/cmark-gfm.git
    GIT_TAG 0.29.0.gfm.12
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(cmark-gfm)

  set(_cmark_gfm_source_dir "${cmark-gfm_SOURCE_DIR}" PARENT_SCOPE)
  set(_cmark_gfm_build_dir "${cmark-gfm_BINARY_DIR}" PARENT_SCOPE)
endfunction()

function(terminal_ui_kit_require_tree_sitter)
  find_package(tree-sitter CONFIG QUIET)
  if(TARGET tree-sitter::tree-sitter)
    return()
  endif()

  message(STATUS "tree-sitter not found via find_package(); fetching with FetchContent")
  FetchContent_Declare(
    tree-sitter
    GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter.git
    GIT_TAG v0.25.3
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(tree-sitter)

  # tree-sitter's CMakeLists.txt is in lib/ subdirectory
  # If FetchContent didn't find it, build manually
  if(NOT TARGET tree-sitter)
    add_subdirectory("${tree-sitter_SOURCE_DIR}/lib" "${tree-sitter_BINARY_DIR}/lib")
  endif()

  set(_tree_sitter_source_dir "${tree-sitter_SOURCE_DIR}" PARENT_SCOPE)
endfunction()

# Helper to fetch a tree-sitter grammar and create a static library target.
# Usage: _terminal_ui_kit_fetch_grammar(c https://github.com/tree-sitter/tree-sitter-c.git v0.23.4)
# The optional 4th argument specifies a subdirectory within the cloned repo that
# contains the grammar's src/ (e.g. "tree-sitter-markdown").
function(_terminal_ui_kit_fetch_grammar name repo tag)
  string(REPLACE "-" "_" safe_name "${name}")
  set(target_name "ts_grammar_${safe_name}")

  if(TARGET ${target_name})
    return()
  endif()

  set(src_dir "${CMAKE_BINARY_DIR}/_ts_grammars/${name}")
  # Some grammars (e.g. tree-sitter-grammars/tree-sitter-markdown) keep
  # their parser.c in a subdirectory of the cloned repository. Resolve the
  # grammar source root before checking whether the checkout is complete, so
  # a valid nested checkout is not cloned again on every configure.
  set(grammar_src_root "${src_dir}")
  if(ARGC GREATER 3)
    set(grammar_src_root "${src_dir}/${ARGV3}")
  endif()

  if(NOT EXISTS "${grammar_src_root}/src/parser.c")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/_ts_grammars")
    message(STATUS "Fetching tree-sitter grammar: ${name}")
    execute_process(
      COMMAND env GIT_TERMINAL_PROMPT=0 git clone --depth 1 --branch ${tag} ${repo} ${src_dir}
      RESULT_VARIABLE result
      TIMEOUT 30)
    if(NOT result EQUAL 0)
      message(WARNING "Failed to fetch tree-sitter grammar: ${name} (skipping)")
      return()
    endif()
  endif()

  file(GLOB grammar_sources "${grammar_src_root}/src/*.c")
  if(grammar_sources)
    add_library(${target_name} STATIC ${grammar_sources})
    target_include_directories(${target_name} PUBLIC "${grammar_src_root}/src")
    set_target_properties(${target_name} PROPERTIES
      C_STANDARD 11
      C_STANDARD_REQUIRED ON
      POSITION_INDEPENDENT_CODE ON)
    target_compile_options(${target_name} PRIVATE -w -fPIC)
  endif()
endfunction()

function(terminal_ui_kit_require_tree_sitter_grammars)
  _terminal_ui_kit_fetch_grammar(c
    https://github.com/tree-sitter/tree-sitter-c.git v0.24.2)
  _terminal_ui_kit_fetch_grammar(cpp
    https://github.com/tree-sitter/tree-sitter-cpp.git v0.23.4)
  _terminal_ui_kit_fetch_grammar(python
    https://github.com/tree-sitter/tree-sitter-python.git v0.25.0)
  _terminal_ui_kit_fetch_grammar(json
    https://github.com/tree-sitter/tree-sitter-json.git v0.24.8)
  _terminal_ui_kit_fetch_grammar(yaml
    https://github.com/tree-sitter-grammars/tree-sitter-yaml.git v0.7.2)
  _terminal_ui_kit_fetch_grammar(bash
    https://github.com/tree-sitter/tree-sitter-bash.git v0.25.1)
  _terminal_ui_kit_fetch_grammar(markdown
    https://github.com/tree-sitter-grammars/tree-sitter-markdown.git v0.5.3
    tree-sitter-markdown)
  _terminal_ui_kit_fetch_grammar(diff
    https://github.com/tree-sitter-grammars/tree-sitter-diff.git v0.1.0)
  _terminal_ui_kit_fetch_grammar(rust
    https://github.com/tree-sitter/tree-sitter-rust.git v0.24.2)
  _terminal_ui_kit_fetch_grammar(javascript
    https://github.com/tree-sitter/tree-sitter-javascript.git v0.25.0)
endfunction()
