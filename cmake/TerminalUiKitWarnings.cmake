# target_terminal_ui_kit_warnings(<target>)
#
# Applies the project's baseline warning flags to <target>. Kept as a single
# function so every module configures warnings identically (see PRD section
# 47, "Coding Style").

function(target_terminal_ui_kit_warnings target)
  if(MSVC)
    set(warning_flags /W4)
    if(TERMINAL_UI_KIT_WARNINGS_AS_ERRORS)
      list(APPEND warning_flags /WX)
    endif()
  else()
    set(warning_flags -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
    if(TERMINAL_UI_KIT_WARNINGS_AS_ERRORS)
      list(APPEND warning_flags -Werror)
    endif()
  endif()

  target_compile_options(${target} PRIVATE ${warning_flags})

  if(TERMINAL_UI_KIT_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
  endif()

  if(TERMINAL_UI_KIT_ENABLE_CLANG_TIDY)
    find_program(TERMINAL_UI_KIT_CLANG_TIDY_EXE NAMES clang-tidy)
    if(TERMINAL_UI_KIT_CLANG_TIDY_EXE)
      set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY "${TERMINAL_UI_KIT_CLANG_TIDY_EXE}")
    else()
      message(WARNING "TERMINAL_UI_KIT_ENABLE_CLANG_TIDY is ON but clang-tidy was not found")
    endif()
  endif()
endfunction()
