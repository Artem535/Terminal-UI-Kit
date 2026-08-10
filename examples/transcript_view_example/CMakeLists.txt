add_executable(terminal_ui_kit_example_transcript_view transcript_view_example.cpp)

target_link_libraries(terminal_ui_kit_example_transcript_view PRIVATE
  TerminalUiKit::Components
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_terminal_ui_kit_warnings(terminal_ui_kit_example_transcript_view)