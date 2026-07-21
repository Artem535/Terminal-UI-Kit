add_executable(terminal_ui_kit_rendering_tests
  virtual_screen_test.cc
  style_bridge_test.cc
  progress_bar_test.cc
  status_indicator_test.cc
  key_hint_bar_test.cc
  spinner_test.cc
  collapsible_panel_test.cc
  modal_stack_test.cc)

target_link_libraries(terminal_ui_kit_rendering_tests PRIVATE
  TerminalUiKit::Testing
  TerminalUiKit::Components
  GTest::gtest_main)

target_terminal_ui_kit_warnings(terminal_ui_kit_rendering_tests)
gtest_discover_tests(terminal_ui_kit_rendering_tests)
