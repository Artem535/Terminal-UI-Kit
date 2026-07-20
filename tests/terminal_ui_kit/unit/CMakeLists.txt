add_executable(terminal_ui_kit_unit_tests
  text_position_test.cc
  text_range_test.cc
  styled_text_test.cc
  text_wrap_test.cc)

target_link_libraries(terminal_ui_kit_unit_tests PRIVATE
  TerminalUiKit::Core
  GTest::gtest_main)

target_terminal_ui_kit_warnings(terminal_ui_kit_unit_tests)
gtest_discover_tests(terminal_ui_kit_unit_tests)
