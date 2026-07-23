add_executable(terminal_ui_kit_rendering_tests
  virtual_screen_test.cc
  virtual_list_test.cc
  style_bridge_test.cc
  progress_bar_test.cc
  indeterminate_progress_test.cc
  progress_tree_test.cc
  status_indicator_test.cc
  key_hint_bar_test.cc
  spinner_test.cc
  collapsible_panel_test.cc
  modal_stack_test.cc
  log_view_test.cc
  virtual_document_test.cc)

if(TERMINAL_UI_KIT_ENABLE_MARKDOWN)
  target_sources(terminal_ui_kit_rendering_tests PRIVATE markdown_view_test.cc)
endif()

target_link_libraries(terminal_ui_kit_rendering_tests PRIVATE
  TerminalUiKit::Testing
  TerminalUiKit::Components
  TerminalUiKit::Document
  GTest::gtest_main)

if(TERMINAL_UI_KIT_ENABLE_MARKDOWN)
  target_link_libraries(terminal_ui_kit_rendering_tests PRIVATE TerminalUiKit::Markdown)
endif()

target_terminal_ui_kit_warnings(terminal_ui_kit_rendering_tests)
gtest_discover_tests(terminal_ui_kit_rendering_tests)
