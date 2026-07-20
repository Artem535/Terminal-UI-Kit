# Module targets for Terminal UI Kit (PRD section 10, "Target CMake Targets").
#
# No module has implementation sources yet, so every target below is
# INTERFACE-only. As modules gain .cc files, add_library(... INTERFACE) will
# become add_library(...) and target_terminal_ui_kit_warnings() should be
# applied (see cmake/TerminalUiKitWarnings.cmake) -- that function requires a
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

_terminal_ui_kit_add_module(core)
_terminal_ui_kit_add_module(components)
_terminal_ui_kit_add_module(document)
_terminal_ui_kit_add_module(editor)
_terminal_ui_kit_add_module(diff)
_terminal_ui_kit_add_module(markdown)
_terminal_ui_kit_add_module(syntax)
_terminal_ui_kit_add_module(terminal)

target_link_libraries(terminal_ui_kit_components INTERFACE terminal_ui_kit_core)
target_link_libraries(terminal_ui_kit_document INTERFACE terminal_ui_kit_core)
target_link_libraries(terminal_ui_kit_editor INTERFACE terminal_ui_kit_document)
target_link_libraries(terminal_ui_kit_diff INTERFACE terminal_ui_kit_core)
target_link_libraries(terminal_ui_kit_markdown INTERFACE terminal_ui_kit_document)
target_link_libraries(terminal_ui_kit_syntax INTERFACE terminal_ui_kit_core)

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
