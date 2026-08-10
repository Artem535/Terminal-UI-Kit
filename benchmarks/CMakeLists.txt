terminal_ui_kit_require_benchmark()
# The ftxui::screen/dom/component imported targets are directory-scoped to
# wherever require_ftxui() ran (src/). Re-require here so the benchmark
# executables can resolve them in this directory scope.
terminal_ui_kit_require_ftxui()

add_executable(terminal_ui_kit_virtual_list_benchmark virtual_list_benchmark.cc)

target_link_libraries(terminal_ui_kit_virtual_list_benchmark PRIVATE
  TerminalUiKit::Components
  benchmark::benchmark
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_terminal_ui_kit_warnings(terminal_ui_kit_virtual_list_benchmark)

add_executable(terminal_ui_kit_unified_diff_view_benchmark unified_diff_view_benchmark.cc)

target_link_libraries(terminal_ui_kit_unified_diff_view_benchmark PRIVATE
  TerminalUiKit::Components
  TerminalUiKit::Diff
  benchmark::benchmark
  ftxui::screen
  ftxui::dom
  ftxui::component)

target_terminal_ui_kit_warnings(terminal_ui_kit_unified_diff_view_benchmark)
