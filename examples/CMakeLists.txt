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

# CompletionPopup is a single-file example (examples/completion_popup_example.cpp).
add_executable(terminal_ui_kit_example_completion_popup completion_popup_example.cpp)
target_link_libraries(terminal_ui_kit_example_completion_popup PRIVATE
  TerminalUiKit::Components
  ftxui::screen
  ftxui::dom
  ftxui::component)
target_terminal_ui_kit_warnings(terminal_ui_kit_example_completion_popup)

if(TERMINAL_UI_KIT_ENABLE_MARKDOWN)
  add_subdirectory(markdown_viewer)
endif()
