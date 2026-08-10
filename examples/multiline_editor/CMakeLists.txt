add_executable(terminal_ui_kit_example_multiline_editor main.cc)

target_link_libraries(terminal_ui_kit_example_multiline_editor PRIVATE
  TerminalUiKit::Editor
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_terminal_ui_kit_warnings(terminal_ui_kit_example_multiline_editor)
