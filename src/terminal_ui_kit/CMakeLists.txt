# Module targets for Terminal UI Kit (PRD section 10, "Target CMake Targets").
#
# Most modules have no implementation sources yet, so they stay
# INTERFACE-only. As a module gains .cc files (see Core and Components
# below for the pattern), add_library(... INTERFACE) becomes
# add_library(...) and target_terminal_ui_kit_warnings() should be applied
# (see cmake/TerminalUiKitWarnings.cmake) -- that function requires a
# non-INTERFACE target.
#
# Theme (include/terminal_ui_kit/theme/) has no dedicated CMake target and is
# exposed through TerminalUiKit::Core, matching the target list in PRD
# section 10, which does not list a separate Theme target.
#
# Images (PRD section 10) has no dedicated include/ directory in the
# repository layout from PRD section 9. Until one is introduced, it is
# implemented as an INTERFACE target that shares the Terminal module's
# headers, per the section 10 note that targets "may ... share a physical
# library".

set(_terminal_ui_kit_include_dir "${PROJECT_SOURCE_DIR}/include")

function(_terminal_ui_kit_add_module name)
  set(target "terminal_ui_kit_${name}")
  string(SUBSTRING "${name}" 0 1 _first)
  string(SUBSTRING "${name}" 1 -1 _rest)
  string(TOUPPER "${_first}" _first)
  set(alias "${_first}${_rest}")

  add_library(${target} INTERFACE)
  add_library(TerminalUiKit::${alias} ALIAS ${target})
  target_include_directories(${target} INTERFACE
    $<BUILD_INTERFACE:${_terminal_ui_kit_include_dir}>
    $<INSTALL_INTERFACE:include>)
  target_compile_features(${target} INTERFACE cxx_std_20)

  set_property(GLOBAL APPEND PROPERTY TERMINAL_UI_KIT_MODULE_TARGETS ${target})
  set(TERMINAL_UI_KIT_TARGET_${alias} ${target} PARENT_SCOPE)
endfunction()

# Core has real implementation sources (text_wrap.cc), so it is a compiled
# library rather than an INTERFACE target like the still-header-only
# modules below.
add_library(terminal_ui_kit_core core/text_wrap.cc core/selection.cc theme/theme.cc)
add_library(TerminalUiKit::Core ALIAS terminal_ui_kit_core)
target_include_directories(terminal_ui_kit_core PUBLIC
  $<BUILD_INTERFACE:${_terminal_ui_kit_include_dir}>
  $<INSTALL_INTERFACE:include>)
target_compile_features(terminal_ui_kit_core PUBLIC cxx_std_20)
target_terminal_ui_kit_warnings(terminal_ui_kit_core)
set_property(GLOBAL APPEND PROPERTY TERMINAL_UI_KIT_MODULE_TARGETS terminal_ui_kit_core)

# Components has real implementation sources and, unlike the header-only
# modules below, unconditionally needs FTXUI: every component returns an
# ftxui::Element or ftxui::Component (PRD section 8.1), so there is no
# tests/examples-only path the way Testing has below. This means a bare
# `cmake -S . -B build` (no tests, no examples) now fetches FTXUI too --
# an accepted, unavoidable consequence of implementing real components,
# not an oversight.
terminal_ui_kit_require_ftxui()
add_library(terminal_ui_kit_components
  components/style_bridge.cc
  components/status_indicator.cc
  components/key_hint_bar.cc
  components/spinner.cc
  components/collapsible_panel.cc
  components/modal_stack.cc
  components/progress_bar.cc
  components/indeterminate_progress.cc
  components/progress_tree.cc
  components/virtual_list.cc
  components/log_view.cc
  components/virtual_document.cc
  components/code_view.cc)
add_library(TerminalUiKit::Components ALIAS terminal_ui_kit_components)
target_include_directories(terminal_ui_kit_components PUBLIC
  $<BUILD_INTERFACE:${_terminal_ui_kit_include_dir}>
  $<INSTALL_INTERFACE:include>)
target_compile_features(terminal_ui_kit_components PUBLIC cxx_std_20)
target_terminal_ui_kit_warnings(terminal_ui_kit_components)
target_link_libraries(terminal_ui_kit_components PUBLIC
  terminal_ui_kit_core
  terminal_ui_kit_document
  ftxui::dom
  ftxui::component
  ftxui::screen)
set_property(GLOBAL APPEND PROPERTY TERMINAL_UI_KIT_MODULE_TARGETS terminal_ui_kit_components)

add_library(terminal_ui_kit_document
  document/streaming_document.cc
  document/ansi_parser.cc
  document/log_model.cc
  document/wrapped_document.cc)
add_library(TerminalUiKit::Document ALIAS terminal_ui_kit_document)
target_include_directories(terminal_ui_kit_document PUBLIC
  $<BUILD_INTERFACE:${_terminal_ui_kit_include_dir}>
  $<INSTALL_INTERFACE:include>)
target_compile_features(terminal_ui_kit_document PUBLIC cxx_std_20)
target_terminal_ui_kit_warnings(terminal_ui_kit_document)
target_link_libraries(terminal_ui_kit_document PUBLIC terminal_ui_kit_core)
set_property(GLOBAL APPEND PROPERTY TERMINAL_UI_KIT_MODULE_TARGETS terminal_ui_kit_document)
_terminal_ui_kit_add_module(editor)
_terminal_ui_kit_add_module(diff)
_terminal_ui_kit_add_module(syntax)
_terminal_ui_kit_add_module(terminal)

target_link_libraries(terminal_ui_kit_editor INTERFACE terminal_ui_kit_document)
target_link_libraries(terminal_ui_kit_diff INTERFACE terminal_ui_kit_core)
target_link_libraries(terminal_ui_kit_syntax INTERFACE terminal_ui_kit_core)

# Markdown module: compiled library when ENABLE_MARKDOWN is ON, otherwise INTERFACE.
if(TERMINAL_UI_KIT_ENABLE_MARKDOWN)
  terminal_ui_kit_require_cmark_gfm()
  add_library(terminal_ui_kit_markdown
    markdown/markdown_document.cc
    markdown/markdown_view.cc)
  add_library(TerminalUiKit::Markdown ALIAS terminal_ui_kit_markdown)
  target_include_directories(terminal_ui_kit_markdown PUBLIC
    $<BUILD_INTERFACE:${_terminal_ui_kit_include_dir}>
    $<INSTALL_INTERFACE:include>)
  target_compile_features(terminal_ui_kit_markdown PUBLIC cxx_std_20)
  target_terminal_ui_kit_warnings(terminal_ui_kit_markdown)
  target_link_libraries(terminal_ui_kit_markdown PUBLIC
    terminal_ui_kit_core
    terminal_ui_kit_components
    terminal_ui_kit_document)
  if(TARGET cmark-gfm::cmark-gfm)
    target_link_libraries(terminal_ui_kit_markdown PUBLIC cmark-gfm::cmark-gfm)
  elseif(TARGET cmark::cmark)
    target_link_libraries(terminal_ui_kit_markdown PUBLIC cmark::cmark)
  elseif(TARGET libcmark-gfm_static)
    if(TARGET libcmark-gfm-extensions_static)
      target_link_libraries(terminal_ui_kit_markdown PUBLIC libcmark-gfm-extensions_static libcmark-gfm_static)
    else()
      target_link_libraries(terminal_ui_kit_markdown PUBLIC libcmark-gfm_static)
    endif()
    target_include_directories(terminal_ui_kit_markdown PUBLIC
      $<BUILD_INTERFACE:${_cmark_gfm_source_dir}/src>
      $<BUILD_INTERFACE:${_cmark_gfm_source_dir}/extensions>
      $<BUILD_INTERFACE:${_cmark_gfm_build_dir}/src>)
  elseif(TARGET libcmark-gfm)
    if(TARGET libcmark-gfm-extensions)
      target_link_libraries(terminal_ui_kit_markdown PUBLIC libcmark-gfm-extensions libcmark-gfm)
    else()
      target_link_libraries(terminal_ui_kit_markdown PUBLIC libcmark-gfm)
    endif()
    target_include_directories(terminal_ui_kit_markdown PUBLIC
      $<BUILD_INTERFACE:${_cmark_gfm_source_dir}/src>
      $<BUILD_INTERFACE:${_cmark_gfm_source_dir}/extensions>
      $<BUILD_INTERFACE:${_cmark_gfm_build_dir}/src>)
  endif()
  set_property(GLOBAL APPEND PROPERTY TERMINAL_UI_KIT_MODULE_TARGETS terminal_ui_kit_markdown)
else()
  _terminal_ui_kit_add_module(markdown)
  target_link_libraries(terminal_ui_kit_markdown INTERFACE terminal_ui_kit_document)
endif()

# Images shares the Terminal module's headers (see file header comment).
add_library(terminal_ui_kit_images INTERFACE)
add_library(TerminalUiKit::Images ALIAS terminal_ui_kit_images)
target_link_libraries(terminal_ui_kit_images INTERFACE terminal_ui_kit_terminal)

add_library(terminal_ui_kit_testing INTERFACE)
add_library(TerminalUiKit::Testing ALIAS terminal_ui_kit_testing)
target_include_directories(terminal_ui_kit_testing INTERFACE
  $<BUILD_INTERFACE:${_terminal_ui_kit_include_dir}>
  $<INSTALL_INTERFACE:include>)
target_compile_features(terminal_ui_kit_testing INTERFACE cxx_std_20)
target_link_libraries(terminal_ui_kit_testing INTERFACE terminal_ui_kit_core)

# terminal_ui_kit/testing/virtual_screen.h renders against real FTXUI types,
# so this target needs FTXUI. Only resolved when tests or examples are
# actually being built; Components already requires FTXUI during a bare
# configure.
if(TERMINAL_UI_KIT_BUILD_TESTS OR TERMINAL_UI_KIT_BUILD_EXAMPLES)
  terminal_ui_kit_require_ftxui()
  target_link_libraries(terminal_ui_kit_testing INTERFACE ftxui::screen ftxui::dom ftxui::component)
endif()

add_library(terminal_ui_kit_all INTERFACE)
add_library(TerminalUiKit::All ALIAS terminal_ui_kit_all)
target_link_libraries(terminal_ui_kit_all INTERFACE
  terminal_ui_kit_core
  terminal_ui_kit_components
  terminal_ui_kit_document
  terminal_ui_kit_editor
  terminal_ui_kit_diff
  terminal_ui_kit_markdown
  terminal_ui_kit_syntax
  terminal_ui_kit_terminal
  terminal_ui_kit_images)

set(TERMINAL_UI_KIT_INSTALL_TARGETS
  terminal_ui_kit_core
  terminal_ui_kit_components
  terminal_ui_kit_document
  terminal_ui_kit_editor
  terminal_ui_kit_diff
  terminal_ui_kit_markdown
  terminal_ui_kit_syntax
  terminal_ui_kit_terminal
  terminal_ui_kit_images
  terminal_ui_kit_testing
  terminal_ui_kit_all
  PARENT_SCOPE)
