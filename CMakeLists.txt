cmake_minimum_required(VERSION 3.21)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" TERMINAL_UI_KIT_VERSION)

project(TerminalUiKit
  VERSION ${TERMINAL_UI_KIT_VERSION}
  DESCRIPTION "High-level C++ components for terminal applications, built on FTXUI"
  HOMEPAGE_URL "https://github.com/Artem535/terminal_ui_kit"
  LANGUAGES CXX)

if(PROJECT_IS_TOP_LEVEL)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(TerminalUiKitOptions)
include(TerminalUiKitWarnings)
include(TerminalUiKitDependencies)

add_subdirectory(src/terminal_ui_kit)

if(TERMINAL_UI_KIT_BUILD_TESTS)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/terminal_ui_kit/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tests/terminal_ui_kit)
  else()
    message(STATUS "TERMINAL_UI_KIT_BUILD_TESTS is ON but tests/terminal_ui_kit has no CMakeLists.txt yet")
  endif()
endif()

if(TERMINAL_UI_KIT_BUILD_EXAMPLES)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
  else()
    message(STATUS "TERMINAL_UI_KIT_BUILD_EXAMPLES is ON but examples has no CMakeLists.txt yet")
  endif()
endif()

if(TERMINAL_UI_KIT_BUILD_BENCHMARKS)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/CMakeLists.txt")
    add_subdirectory(benchmarks)
  else()
    message(STATUS "TERMINAL_UI_KIT_BUILD_BENCHMARKS is ON but benchmarks has no CMakeLists.txt yet")
  endif()
endif()

# --- Install and package export (find_package(TerminalUiKit) support) -----

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS ${TERMINAL_UI_KIT_INSTALL_TARGETS}
  EXPORT TerminalUiKitTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY include/terminal_ui_kit
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h")

install(EXPORT TerminalUiKitTargets
  NAMESPACE TerminalUiKit::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TerminalUiKit)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/TerminalUiKitConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/TerminalUiKitConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TerminalUiKit)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/TerminalUiKitConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/TerminalUiKitConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/TerminalUiKitConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TerminalUiKit)
