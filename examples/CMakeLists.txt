# Example applications demonstrating library components in isolation (PRD
# section 54). Each subdirectory is a standalone add_executable target; add
# a new add_subdirectory(...) line here as each future task adds its own
# example.
#
# The ftxui::screen/dom/component imported targets are directory-scoped to
# wherever require_ftxui() ran (src/). Re-require here so example executables
# that link FTXUI directly can resolve them in this directory scope.
terminal_ui_kit_require_ftxui()

add_subdirectory(theme_viewer)
add_subdirectory(components_gallery)
add_subdirectory(progress_viewer)
add_subdirectory(task_dashboard)
add_subdirectory(virtual_list_viewer)
add_subdirectory(streaming_log_viewer)
add_subdirectory(virtual_document_viewer)
add_subdirectory(searchable_text_view_example)
add_subdirectory(diff_parser)
add_subdirectory(multiline_editor)
add_subdirectory(toast_example)
add_subdirectory(command_history_example)

# Standalone terminal-capabilities example. FTXUI is pulled transitively via
# TerminalUiKit::Components so the ftxui imported targets remain visible under
# CMake >= 4 (find_package scopes imported targets to their calling directory).
add_executable(terminal_ui_kit_example_terminal_capabilities
  terminal_capabilities_example.cpp)
target_link_libraries(terminal_ui_kit_example_terminal_capabilities PRIVATE
  TerminalUiKit::Components
  TerminalUiKit::Terminal)
target_terminal_ui_kit_warnings(terminal_ui_kit_example_terminal_capabilities)

# Standalone completion-popup example. FTXUI is pulled transitively via
# TerminalUiKit::Components so the ftxui imported targets remain visible under
# CMake >= 4 (find_package scopes imported targets to their calling directory).
add_executable(terminal_ui_kit_example_completion_popup
  completion_popup_example.cpp)
target_link_libraries(terminal_ui_kit_example_completion_popup PRIVATE
  TerminalUiKit::Components)
target_terminal_ui_kit_warnings(terminal_ui_kit_example_completion_popup)

# UnifiedDiffView example lives directly in examples/ (single-file target).
add_executable(terminal_ui_kit_example_unified_diff_view unified_diff_view_example.cpp)
target_link_libraries(terminal_ui_kit_example_unified_diff_view PRIVATE
  TerminalUiKit::Components
  TerminalUiKit::Diff
  ftxui::screen
  ftxui::dom
  ftxui::component)
target_terminal_ui_kit_warnings(terminal_ui_kit_example_unified_diff_view)

if(TERMINAL_UI_KIT_ENABLE_MARKDOWN)
  add_subdirectory(markdown_viewer)
endif()

# Standalone example demonstrating the line-number gutter formatting fix
# (single-file example kept at the examples/ root).
add_executable(terminal_ui_kit_example_line_number_formatting
  line_number_formatting_example.cpp)
target_link_libraries(terminal_ui_kit_example_line_number_formatting PRIVATE
  TerminalUiKit::Core
  TerminalUiKit::Components)
target_terminal_ui_kit_warnings(terminal_ui_kit_example_line_number_formatting)
