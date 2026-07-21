add_executable(terminal_ui_kit_example_components_gallery main.cc)

target_link_libraries(terminal_ui_kit_example_components_gallery PRIVATE
  TerminalUiKit::Components
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_terminal_ui_kit_warnings(terminal_ui_kit_example_components_gallery)
