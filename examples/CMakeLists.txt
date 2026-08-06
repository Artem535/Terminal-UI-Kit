# Example applications demonstrating library components in isolation (PRD
# section 54). Each subdirectory is a standalone add_executable target; add
# a new add_subdirectory(...) line here as each future task adds its own
# example.
#
# FTXUI is imported in this directory's scope: the FTXUI targets are created by
# terminal_ui_kit_require_ftxui() inside src/terminal_ui_kit (a sibling
# directory), and imported targets from find_package() are not visible across
# sibling directory scopes. Examples link ftxui::dom/component/screen directly,
# so import it here too.
terminal_ui_kit_require_ftxui()

add_subdirectory(theme_viewer)
add_subdirectory(components_gallery)
add_subdirectory(progress_viewer)
add_subdirectory(task_dashboard)
add_subdirectory(virtual_list_viewer)
add_subdirectory(streaming_log_viewer)
add_subdirectory(virtual_document_viewer)
add_subdirectory(diff_parser)

# CommandHistory interactive demo: a header-only model example registered as a
# standalone executable source (see examples/command_history_example.cpp).
add_executable(terminal_ui_kit_example_command_history
  command_history_example.cpp)
target_link_libraries(terminal_ui_kit_example_command_history PRIVATE
  TerminalUiKit::Command
  ftxui::screen
  ftxui::dom
  ftxui::component)
target_terminal_ui_kit_warnings(terminal_ui_kit_example_command_history)

if(TERMINAL_UI_KIT_ENABLE_MARKDOWN)
  add_subdirectory(markdown_viewer)
endif()
