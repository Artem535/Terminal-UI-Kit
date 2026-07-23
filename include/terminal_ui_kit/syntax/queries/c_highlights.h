#pragma once

namespace terminal_ui_kit::syntax_queries {

constexpr const char* kCHighlights = R"QUERY(
; Keywords
"if" @keyword
"else" @keyword
"switch" @keyword
"case" @keyword
"default" @keyword
"break" @keyword
"continue" @keyword
"do" @keyword
"for" @keyword
"while" @keyword
"goto" @keyword
"return" @keyword.return
"struct" @keyword.type
"union" @keyword.type
"enum" @keyword.type
"typedef" @keyword.type
"sizeof" @keyword
"alignof" @keyword
"__attribute__" @attribute

; Storage class specifiers
"static" @keyword
"extern" @keyword
"register" @keyword
"inline" @keyword
"const" @keyword
"volatile" @keyword
"restrict" @keyword

; Primitive types
(primitive_type) @type.builtin

; Type qualifiers
(type_qualifier) @keyword

; Function definitions and calls
(function_declarator
  declarator: (identifier) @function)

(call_expression
  function: (identifier) @function)

(call_expression
  function: (field_expression
    field: (field_identifier) @method))

; Function definitions with pointer declarator
(function_declarator
  declarator: (pointer_declarator
    declarator: (identifier) @function))

; Parameters
(parameter_declaration
  declarator: (identifier) @parameter)

(parameter_declaration
  declarator: (pointer_declarator
    declarator: (identifier) @parameter))

; Struct/union fields
(field_identifier) @field

; Labels
(labeled_statement
  (statement_identifier) @label)

(goto_statement
  (statement_identifier) @label)

; Strings
(string_literal) @string
(system_lib_string) @string

; Escape sequences
(escape_sequence) @escape

; Numbers
(number_literal) @number

; Comments
(comment) @comment

; Operators
"+" @operator
"-" @operator
"*" @operator
"/" @operator
"%" @operator
"++" @operator
"--" @operator
"=" @operator
"+=" @operator
"-=" @operator
"*=" @operator
"/=" @operator
"%=" @operator
"&=" @operator
"|=" @operator
"^=" @operator
"<<=" @operator
">>=" @operator
"==" @operator
"!=" @operator
"<" @operator
">" @operator
"<=" @operator
">=" @operator
"&&" @operator
"||" @operator
"!" @operator
"&" @operator
"|" @operator
"^" @operator
"~" @operator
"<<" @operator
">>" @operator
"?" @operator
":" @operator

; Punctuation
"(" @punctuation.bracket
")" @punctuation.bracket
"[" @punctuation.bracket
"]" @punctuation.bracket
"{" @punctuation.bracket
"}" @punctuation.bracket
";" @punctuation.delimiter
"," @punctuation.delimiter
"." @punctuation.delimiter
"->" @punctuation.delimiter

; Constants
((identifier) @constant.builtin
  (#match? @constant.builtin "^NULL$"))

; Preprocessor
(preproc_include) @import
(preproc_def) @macro
(preproc_function_def) @macro
(preproc_ifdef) @macro
(preproc_if) @macro
(preproc_else) @macro
(preproc_elif) @macro
(preproc_endif) @macro
(preproc_defined) @macro

; Identifiers
(identifier) @variable
)QUERY";

}  // namespace terminal_ui_kit::syntax_queries
