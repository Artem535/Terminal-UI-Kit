#pragma once

namespace terminal_ui_kit::syntax_queries {

constexpr const char* kYamlHighlights = R"QUERY(
; Keys
(block_mapping_pair
  key: (flow_node
    (plain_scalar
      (string_scalar) @property)))

(block_mapping_pair
  key: (flow_node
    (double_quote_scalar) @property))

(block_mapping_pair
  key: (flow_node
    (single_quote_scalar) @property))

(flow_mapping
  (flow_pair
    key: (flow_node
      (plain_scalar
        (string_scalar) @property))))

(flow_mapping
  (flow_pair
    key: (flow_node
      (double_quote_scalar) @property)))

(flow_mapping
  (flow_pair
    key: (flow_node
      (single_quote_scalar) @property)))

; Strings
(string_scalar) @string
(double_quote_scalar) @string
(single_quote_scalar) @string
(block_scalar) @string

; Escape sequences
(escape_sequence) @escape

; Numbers
(integer_scalar) @number
(float_scalar) @float

; Boolean constants
((boolean_scalar) @constant.builtin
  (#match? @constant.builtin "^(true|false|True|False|TRUE|FALSE|yes|no|Yes|No|YES|NO|on|off|On|Off|ON|OFF)$"))

; Null constant
((null_scalar) @constant.builtin
  (#match? @constant.builtin "^(null|Null|NULL|~)$"))

; Comments
(comment) @comment

; Anchors and aliases
(anchor) @label
(alias) @label
"&" @label
"*" @label

; Tags
(tag) @type

; Directives
"---" @punctuation.delimiter
"..." @punctuation.delimiter

; Punctuation
":" @punctuation.delimiter
"," @punctuation.delimiter
"[" @punctuation.bracket
"]" @punctuation.bracket
"{" @punctuation.bracket
"}" @punctuation.bracket
"|" @operator
">" @operator
)QUERY";

}  // namespace terminal_ui_kit::syntax_queries
