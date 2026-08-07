terminal_ui_kit_require_benchmark()

add_executable(terminal_ui_kit_virtual_list_benchmark virtual_list_benchmark.cc)

target_link_libraries(terminal_ui_kit_virtual_list_benchmark PRIVATE
  TerminalUiKit::Components
  benchmark::benchmark
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_terminal_ui_kit_warnings(terminal_ui_kit_virtual_list_benchmark)

add_executable(terminal_ui_kit_search_benchmark search_benchmark.cc)

target_link_libraries(terminal_ui_kit_search_benchmark PRIVATE
  TerminalUiKit::Search
  TerminalUiKit::Components
  benchmark::benchmark
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_terminal_ui_kit_warnings(terminal_ui_kit_search_benchmark)
