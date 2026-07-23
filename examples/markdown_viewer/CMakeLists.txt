add_executable(terminal_ui_kit_example_markdown_viewer main.cc)

target_link_libraries(terminal_ui_kit_example_markdown_viewer PRIVATE
  TerminalUiKit::Markdown
  TerminalUiKit::Components
  TerminalUiKit::Document
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_terminal_ui_kit_warnings(terminal_ui_kit_example_markdown_viewer)