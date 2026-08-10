terminal_ui_kit_require_benchmark()

add_executable(terminal_ui_kit_virtual_list_benchmark virtual_list_benchmark.cc)

target_link_libraries(terminal_ui_kit_virtual_list_benchmark PRIVATE
  TerminalUiKit::Components
  benchmark::benchmark
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_terminal_ui_kit_warnings(terminal_ui_kit_virtual_list_benchmark)

add_executable(terminal_ui_kit_multiline_editor_benchmark multiline_editor_benchmark.cc)

target_link_libraries(terminal_ui_kit_multiline_editor_benchmark PRIVATE
  TerminalUiKit::Editor
  benchmark::benchmark
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_terminal_ui_kit_warnings(terminal_ui_kit_multiline_editor_benchmark)
