add_executable(terminal_ui_kit_unit_tests
  text_position_test.cc
  text_range_test.cc
  styled_text_test.cc
  text_wrap_test.cc
  theme_test.cc
  streaming_document_test.cc
  ansi_parser_test.cc
  log_model_test.cc
  wrapped_document_test.cc
  selection_manager_test.cc)

if(TERMINAL_UI_KIT_ENABLE_MARKDOWN)
  target_sources(terminal_ui_kit_unit_tests PRIVATE markdown_document_test.cc)
endif()

target_link_libraries(terminal_ui_kit_unit_tests PRIVATE
  TerminalUiKit::Core
  TerminalUiKit::Document
  GTest::gtest_main)

if(TERMINAL_UI_KIT_ENABLE_MARKDOWN)
  target_link_libraries(terminal_ui_kit_unit_tests PRIVATE TerminalUiKit::Markdown)
endif()

target_terminal_ui_kit_warnings(terminal_ui_kit_unit_tests)
gtest_discover_tests(terminal_ui_kit_unit_tests)
