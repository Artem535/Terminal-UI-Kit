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
add_subdirectory(diff_parser)

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
