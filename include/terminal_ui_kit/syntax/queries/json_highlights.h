#pragma once

namespace terminal_ui_kit::syntax_queries {

constexpr const char* kJsonHighlights = R"QUERY(
; Strings (keys and values)
(pair
  key: (string) @property.key)

(string) @string

; Escape sequences
(escape_sequence) @escape

; Numbers
(number) @number

; Boolean constants
((true) @constant.builtin)
((false) @constant.builtin)

; Null constant
((null) @constant.builtin)

; Punctuation
"{" @punctuation.bracket
"}" @punctuation.bracket
"[" @punctuation.bracket
"]" @punctuation.bracket
":" @punctuation.delimiter
"," @punctuation.delimiter
)QUERY";

}  // namespace terminal_ui_kit::syntax_queries
