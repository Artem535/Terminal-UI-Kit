add_executable(terminal_ui_kit_example_command_history main.cc)

# FTXUI is linked transitively through TerminalUiKit::Components (which
# PUBLIC-links ftxui::dom/component/screen), so dependents need not name the
# ftxui targets directly.
target_link_libraries(terminal_ui_kit_example_command_history PRIVATE
  TerminalUiKit::Editor
  TerminalUiKit::Components)

target_terminal_ui_kit_warnings(terminal_ui_kit_example_command_history)
