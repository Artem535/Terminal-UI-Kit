-- Xmake is a secondary, developer-facing build frontend for the Linux
-- workflow (PRD section 11.2). CMake remains authoritative for releases,
-- packaging, and CI installation jobs: library targets in the root xmake.lua
-- are header-only mirrors that do not compile the implementation .cc files.
--
-- The TranscriptView example is registered here for target discoverability
-- alongside its CMake counterpart (`terminal_ui_kit_example_transcript_view`).
-- Because Xmake does not compile the Components implementation, this target is
-- a header-only mirror of the example source + component headers rather than a
-- buildable binary; build and run it via CMake (see examples/README.md).
target("terminal_ui_kit_example_transcript_view")
    set_kind("headeronly")
    add_headerfiles("transcript_view_example/transcript_view_example.cpp",
                    "include/terminal_ui_kit/components/transcript.h",
                    "include/terminal_ui_kit/components/transcript_model.h")
target_end()