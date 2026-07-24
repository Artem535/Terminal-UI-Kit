#pragma once

namespace terminal_ui_kit::syntax_queries {

// Queries for the block-level markdown parser (tree-sitter-markdown).
// Note: inline formatting (bold, italic, code_span, links, images) lives
// in a separate tree-sitter-markdown-inline grammar and is not covered
// by this block-level query set.
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

; Code blocks
(fenced_code_block) @string
(indented_code_block) @string
(code_fence_content) @string
(fenced_code_block_delimiter) @punctuation.delimiter
(language) @type
(info_string) @string

; Link parts (block-level)
(link_destination) @string
(link_label) @string
(link_reference_definition) @keyword
(link_title) @string

; List items
(list_item
  (list_marker_dot) @punctuation.delimiter)

(list_item
  (list_marker_minus) @punctuation.delimiter)

(list_item
  (list_marker_plus) @punctuation.delimiter)

(list_item
  (list_marker_star) @punctuation.delimiter)

(list_item
  (list_marker_parenthesis) @punctuation.delimiter)

; Task list items
(task_list_marker_unchecked) @punctuation.delimiter
(task_list_marker_checked) @constant.builtin

; Block quotes
(block_quote
  (block_quote_marker) @comment)

; Horizontal rules
(thematic_break) @punctuation.delimiter

; HTML blocks
(html_block) @tag

; Tables (GitHub Flavored Markdown)
(pipe_table_header) @keyword
(pipe_table_cell) @property
(pipe_table_delimiter_row) @punctuation.delimiter

; Paragraphs — inline content placeholder
(inline) @comment

; Section
(section) @comment
)QUERY";

}  // namespace terminal_ui_kit::syntax_queries