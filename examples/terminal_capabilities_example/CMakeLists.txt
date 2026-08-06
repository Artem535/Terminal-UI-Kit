add_executable(terminal_ui_kit_example_terminal_capabilities main.cc)

target_link_libraries(terminal_ui_kit_example_terminal_capabilities PRIVATE
  TerminalUiKit::Terminal
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_terminal_ui_kit_warnings(terminal_ui_kit_example_terminal_capabilities)