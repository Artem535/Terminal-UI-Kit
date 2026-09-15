add_executable(terminal_ui_kit_example_searchable_text_view main.cc)
target_link_libraries(terminal_ui_kit_example_searchable_text_view PRIVATE
  TerminalUiKit::Search
  TerminalUiKit::Components
  ftxui::screen ftxui::dom ftxui::component)
target_terminal_ui_kit_warnings(terminal_ui_kit_example_searchable_text_view)