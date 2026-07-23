#pragma once

namespace terminal_ui_kit::syntax_queries {

constexpr const char* kDiffHighlights = R"QUERY(
; File headers
(diff_header) @keyword

; Chunk headers
(chunk_header) @label

; Additions
(line_added) @string

; Deletions
(line_deleted) @comment

; Context lines
(line_context) @comment

; File modes
(file_mode) @type

; Binary files
(binary_change) @constant

; Commit info
(commit) @constant

; Author
(author) @attribute

; Date
(date) @number

; Punctuation
"+" @string
"-" @comment
"@" @label
"=" @keyword
)QUERY";

}  // namespace terminal_ui_kit::syntax_queries
