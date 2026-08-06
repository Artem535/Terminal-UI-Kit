add_executable(terminal_ui_kit_example_line_number_formatting main.cc)

target_link_libraries(terminal_ui_kit_example_line_number_formatting PRIVATE
  TerminalUiKit::Components
  TerminalUiKit::Document
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_terminal_ui_kit_warnings(terminal_ui_kit_example_line_number_formatting)