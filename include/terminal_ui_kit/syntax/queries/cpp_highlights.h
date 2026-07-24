#pragma once

namespace terminal_ui_kit::syntax_queries {

constexpr const char* kCppHighlights = R"QUERY(
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
"co_return" @keyword.return
"co_yield" @keyword
"co_await" @keyword
"throw" @keyword
"try" @keyword
"catch" @keyword
"noexcept" @keyword

; Type keywords
"class" @keyword.type
"struct" @keyword.type
"union" @keyword.type
"enum" @keyword.type
"typedef" @keyword.type
"typename" @keyword.type
"decltype" @keyword.type
"sizeof" @keyword
"alignof" @keyword
"alignas" @keyword

; Access specifiers
"public" @keyword
"private" @keyword
"protected" @keyword
"virtual" @keyword
"override" @keyword
"final" @keyword

; Storage class specifiers
"static" @keyword
"extern" @keyword
"register" @keyword
"inline" @keyword
"constexpr" @keyword
"consteval" @keyword
"constinit" @keyword
"mutable" @keyword
"thread_local" @keyword

; Qualifiers
"const" @keyword
"volatile" @keyword

; Operator keywords
"new" @keyword.operator
"delete" @keyword.operator
"and" @keyword.operator
"or" @keyword.operator
"not" @keyword.operator
"and_eq" @keyword.operator
"or_eq" @keyword.operator
"not_eq" @keyword.operator
"xor" @keyword.operator
"xor_eq" @keyword.operator
"bitand" @keyword.operator
"bitor" @keyword.operator
"compl" @keyword.operator

; Namespace
"namespace" @keyword.type
"using" @keyword

; Template
"template" @keyword
"concept" @keyword
"requires" @keyword

; Primitive types
(primitive_type) @type.builtin

; Namespace definitions and usage
(namespace_definition
  name: (namespace_identifier) @namespace)

(namespace_identifier) @namespace

; Type identifiers
(type_identifier) @type

; Class definitions
(class_specifier
  name: (type_identifier) @type)

; Struct definitions
(struct_specifier
  name: (type_identifier) @type)

; Enum definitions
(enum_specifier
  name: (type_identifier) @type)

; Function definitions and calls
(function_declarator
  declarator: (identifier) @function)

(function_declarator
  declarator: (qualified_identifier
    name: (identifier) @function))

(call_expression
  function: (identifier) @function)

(call_expression
  function: (qualified_identifier
    name: (identifier) @function))

(call_expression
  function: (field_expression
    field: (field_identifier) @method))

; Destructor
(destructor_name) @constructor

; Parameters
(optional_parameter_declaration
  declarator: (identifier) @parameter)

(parameter_declaration
  declarator: (identifier) @parameter)

(parameter_declaration
  declarator: (reference_declarator
    (identifier) @parameter))

(parameter_declaration
  declarator: (pointer_declarator
    declarator: (identifier) @parameter))

; Struct/class fields
(field_identifier) @field

; Labels
(labeled_statement
  (statement_identifier) @label)

(goto_statement
  (statement_identifier) @label)

; Strings
(string_literal) @string
(raw_string_literal) @string
(char_literal) @string

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
"<=>" @operator
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
"::" @operator
"..." @operator

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
".*" @punctuation.delimiter
"->*" @punctuation.delimiter

; Constants
((identifier) @constant.builtin
  (#match? @constant.builtin "^NULL$"))

(true) @constant.builtin
(false) @constant.builtin
"nullptr" @constant.builtin

; Preprocessor
(preproc_include) @import
(preproc_def) @macro
(preproc_function_def) @macro
(preproc_ifdef) @macro
(preproc_if) @macro
(preproc_else) @macro
(preproc_elif) @macro
(preproc_elifdef) @macro
(preproc_defined) @macro
(preproc_call) @macro

; Identifiers
(identifier) @variable
)QUERY";

}  // namespace terminal_ui_kit::syntax_queries
