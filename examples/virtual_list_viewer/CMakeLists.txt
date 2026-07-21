add_executable(terminal_ui_kit_example_virtual_list_viewer main.cc)

target_link_libraries(terminal_ui_kit_example_virtual_list_viewer PRIVATE
  TerminalUiKit::Components
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_terminal_ui_kit_warnings(terminal_ui_kit_example_virtual_list_viewer)
