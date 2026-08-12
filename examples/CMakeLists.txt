# Example applications demonstrating library components in isolation (PRD
# section 54). Each subdirectory is a standalone add_executable target; add
# a new add_subdirectory(...) line here as each future task adds its own
# example.

add_subdirectory(theme_viewer)
add_subdirectory(components_gallery)
add_subdirectory(progress_viewer)
add_subdirectory(task_dashboard)
add_subdirectory(virtual_list_viewer)
add_subdirectory(streaming_log_viewer)
add_subdirectory(virtual_document_viewer)
add_subdirectory(diff_parser)

add_executable(terminal_ui_kit_example_line_number_formatting line_number_formatting_example.cpp)
# Note: FTXUI is pulled in transitively via TerminalUiKit::Components (which
# links ftxui:: screen/dom/component). Linking ftxui::* directly here fails to
# resolve in this directory with CMake >= 4 (find_package inside
# terminal_ui_kit_require_ftxui scopes the imported targets to the components
# directory), so the example relies on Components' transitive linkage instead.
target_link_libraries(terminal_ui_kit_example_line_number_formatting PRIVATE
  TerminalUiKit::Components
  TerminalUiKit::Document)
target_terminal_ui_kit_warnings(terminal_ui_kit_example_line_number_formatting)

if(TERMINAL_UI_KIT_ENABLE_MARKDOWN)
  add_subdirectory(markdown_viewer)
endif()
