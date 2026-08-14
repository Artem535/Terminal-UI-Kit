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

# Standalone terminal-capabilities example. FTXUI is pulled transitively via
# TerminalUiKit::Components so the ftxui imported targets remain visible under
# CMake >= 4 (find_package scopes imported targets to their calling directory).
add_executable(terminal_ui_kit_example_terminal_capabilities
  terminal_capabilities_example.cpp)
target_link_libraries(terminal_ui_kit_example_terminal_capabilities PRIVATE
  TerminalUiKit::Components
  TerminalUiKit::Terminal)
target_terminal_ui_kit_warnings(terminal_ui_kit_example_terminal_capabilities)

if(TERMINAL_UI_KIT_ENABLE_MARKDOWN)
  add_subdirectory(markdown_viewer)
endif()
