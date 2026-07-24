#pragma once

namespace terminal_ui_kit::syntax_queries {

constexpr const char* kBashHighlights = R"QUERY(
; Keywords
"if" @keyword
"then" @keyword
"elif" @keyword
"else" @keyword
"fi" @keyword
"case" @keyword
"in" @keyword
"esac" @keyword
"for" @keyword
"while" @keyword
"until" @keyword
"do" @keyword
"done" @keyword
"select" @keyword
"function" @keyword
"declare" @keyword
"typeset" @keyword
"export" @keyword
"readonly" @keyword
"local" @keyword
"unset" @keyword

; Operator keywords
"!" @keyword.operator
"&&" @keyword.operator
"||" @keyword.operator

; Commands (first word in a command)
(command_name
  (word) @function)

; Built-in commands
((command_name
  (word) @function.builtin)
  (#match? @function.builtin "^(echo|printf|read|cd|pwd|pushd|popd|dirs|let|test|true|false|set|unset|shopt|builtin|command|type|hash|help|logout|umask|wait|jobs|bg|fg|kill|suspend|alias|unalias|bind|complete|compgen|compopt|caller|enable|getopts|mapfile|readarray|return|break|continue|shift|trap|exit|exec|eval|source|coproc|time)$"))

; Function definitions
(function_definition
  name: (word) @function)

; Variables
(variable_name) @variable

; Special variables
(special_variable_name) @variable.builtin

; Variable assignments
(variable_assignment
  name: (variable_name) @variable)

; Strings
(string) @string
(ansi_c_string) @string
(raw_string) @string

; Heredoc
(heredoc_body) @string
(heredoc_start) @string
(heredoc_end) @string

; Comments
(comment) @comment

; Numbers
((word) @number
  (#match? @number "^[0-9]+$"))

; Arithmetic expansion
(arithmetic_expansion) @number

; Test operators
(test_operator) @operator

; Operators
"=" @operator
"==" @operator
"!=" @operator
"+" @operator
"-" @operator
"*" @operator
"/" @operator
"%" @operator
"+=" @operator
"-=" @operator
"*=" @operator
"/=" @operator
"%=" @operator
"++" @operator
"--" @operator
"<" @operator
">" @operator
"<=" @operator
">=" @operator
"&" @operator
"|" @operator
"^" @operator
"~" @operator
"<<" @operator
">>" @operator

; Punctuation
"$(" @punctuation.bracket
"${" @punctuation.bracket
"}" @punctuation.bracket
"{" @punctuation.bracket
"(" @punctuation.bracket
")" @punctuation.bracket
"`" @punctuation.bracket
";" @punctuation.delimiter
";;" @punctuation.delimiter
"," @punctuation.delimiter

; File descriptors
(file_descriptor) @number
)QUERY";

}  // namespace terminal_ui_kit::syntax_queries
