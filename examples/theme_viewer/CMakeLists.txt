add_executable(terminal_ui_kit_example_theme_viewer main.cc)

target_link_libraries(terminal_ui_kit_example_theme_viewer PRIVATE
  TerminalUiKit::Core
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_terminal_ui_kit_warnings(terminal_ui_kit_example_theme_viewer)
