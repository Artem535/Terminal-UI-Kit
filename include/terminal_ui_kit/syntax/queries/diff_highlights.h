#pragma once

namespace terminal_ui_kit::syntax_queries {

constexpr const char* kDiffHighlights = R"QUERY(
; File changes
(file_change) @keyword

; Hunks
(hunk) @label

; Additions
(addition) @string

; Deletions
(deletion) @comment

; Context lines
(context) @comment

; File modes
(mode) @type

; Binary files
(binary_change) @constant

; Commit info
(commit) @constant

; Filenames
(filename) @attribute
(old_file) @attribute
(new_file) @attribute

; Index
(index) @number
)QUERY";

}  // namespace terminal_ui_kit::syntax_queries
