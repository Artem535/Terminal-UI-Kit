add_executable(terminal_ui_kit_example_streaming_log_viewer main.cc)

target_link_libraries(terminal_ui_kit_example_streaming_log_viewer PRIVATE
  TerminalUiKit::Components
  TerminalUiKit::Document
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_terminal_ui_kit_warnings(terminal_ui_kit_example_streaming_log_viewer)