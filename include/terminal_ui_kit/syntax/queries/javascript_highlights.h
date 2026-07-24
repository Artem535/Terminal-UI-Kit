#pragma once

namespace terminal_ui_kit::syntax_queries {

constexpr const char* kJavascriptHighlights = R"QUERY(
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
"return" @keyword.return
"throw" @keyword
"try" @keyword
"catch" @keyword
"finally" @keyword
"yield" @keyword
"async" @keyword
"await" @keyword
"debugger" @keyword
"with" @keyword

; Declarations
"var" @keyword
"let" @keyword
"const" @keyword

; Type/class keywords
"class" @keyword.type
"extends" @keyword.type
"new" @keyword.operator
"delete" @keyword.operator
"typeof" @keyword.operator
"instanceof" @keyword.operator
"void" @keyword.operator

; Import/export
"import" @import
"export" @import
"from" @import
"as" @import

(import_statement
  source: (string) @string)

; Function definitions
(function_declaration
  name: (identifier) @function)

(function_expression
  name: (identifier) @function)

(arrow_function) @function

; Method definitions
(method_definition
  name: (property_identifier) @method)

; Class definitions
(class_declaration
  name: (identifier) @type)

(class
  name: (identifier) @type)

; Calls
(call_expression
  function: (identifier) @function)

(call_expression
  function: (member_expression
    property: (property_identifier) @method))

; New expressions
(new_expression
  constructor: (identifier) @constructor)

; Parameters
(formal_parameters
  (identifier) @parameter)

(formal_parameters
  (assignment_pattern
    left: (identifier) @parameter))

; Property definitions
(pair
  key: (property_identifier) @property.key)

; Shorthand properties
(shorthand_property_identifier) @property.shorthand

; Strings
(string) @string
(template_string) @string
(template_substitution) @string

; Escape sequences
(escape_sequence) @escape

; Numbers
(number) @number

; Comments
(comment) @comment

; Boolean constants
(true) @constant.builtin
(false) @constant.builtin

; Null constant
(null) @constant.builtin

; This
(this) @variable.builtin

; Undefined
(undefined) @variable.builtin

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
"===" @operator
"!=" @operator
"!==" @operator
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
"..." @operator
"=>" @operator

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

; Identifiers
(identifier) @variable
(property_identifier) @property
)QUERY";

}  // namespace terminal_ui_kit::syntax_queries
