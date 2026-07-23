#pragma once

namespace terminal_ui_kit::syntax_queries {

constexpr const char* kMarkdownHighlights = R"QUERY(
; Headings
(atx_heading
  (atx_h1_marker) @keyword)

(atx_heading
  (atx_h2_marker) @keyword)

(atx_heading
  (atx_h3_marker) @keyword)

(atx_heading
  (atx_h4_marker) @keyword)

(atx_heading
  (atx_h5_marker) @keyword)

(atx_heading
  (atx_h6_marker) @keyword)

(setext_heading
  (setext_h1_underline) @keyword)

(setext_heading
  (setext_h2_underline) @keyword)

; Bold
(bold) @keyword

; Italic
(italic) @keyword

; Strikethrough
(strikethrough) @keyword

; Code spans
(code_span) @string

; Code blocks
(fenced_code_block) @string
(indented_code_block) @string
(code_fence_content) @string

; Links
(link_text) @string
(link_destination) @string
(link_label) @string

; Images
(image
  (link_text) @string)
(image
  (link_destination) @string)

; List items
(list_item
  (list_marker) @punctuation.delimiter)

; Task list items
(task_list_marker_unchecked) @punctuation.delimiter
(task_list_marker_checked) @constant.builtin

; Block quotes
(block_quote) @comment

; Horizontal rules
(thematic_break) @punctuation.delimiter

; HTML tags
(html_tag) @tag

; Punctuation
"[" @punctuation.bracket
"]" @punctuation.bracket
"(" @punctuation.bracket
")" @punctuation.bracket
"!" @punctuation.delimiter
"#" @punctuation.delimiter
"*" @punctuation.delimiter
"_" @punctuation.delimiter
"~" @punctuation.delimiter
"`" @punctuation.delimiter
"---" @punctuation.delimiter
"===" @punctuation.delimiter
)QUERY";

}  // namespace terminal_ui_kit::syntax_queries
