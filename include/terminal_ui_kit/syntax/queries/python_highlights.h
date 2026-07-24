#pragma once

namespace terminal_ui_kit::syntax_queries {

constexpr const char* kPythonHighlights = R"QUERY(
; Keywords
"if" @keyword
"elif" @keyword
"else" @keyword
"for" @keyword
"while" @keyword
"break" @keyword
"continue" @keyword
"pass" @keyword
"return" @keyword.return
"yield" @keyword
"del" @keyword
"try" @keyword
"except" @keyword
"finally" @keyword
"raise" @keyword
"assert" @keyword
"with" @keyword
"as" @keyword
"async" @keyword
"await" @keyword
"match" @keyword
"case" @keyword

; Type/class keywords
"class" @keyword.type
"def" @keyword

; Operator keywords
"and" @keyword.operator
"or" @keyword.operator
"not" @keyword.operator
"in" @keyword.operator
"is" @keyword.operator
"lambda" @keyword.operator

; Import statements
"import" @import
"from" @import

(import_from_statement
  module_name: (dotted_name) @namespace)

; Decorators
(decorator) @attribute
(decorator
  (identifier) @attribute)

; Function definitions
(function_definition
  name: (identifier) @function)

; Class definitions
(class_definition
  name: (identifier) @type)

; Calls
(call
  function: (identifier) @function)

(call
  function: (attribute
    attribute: (identifier) @method))

; Parameters
(parameters
  (identifier) @parameter)

(default_parameter
  name: (identifier) @parameter)

(keyword_argument
  name: (identifier) @parameter)

(typed_parameter
  (identifier) @parameter)

; Type annotations
(type) @type

; Strings
(string) @string
(interpolation) @string

; Escape sequences
(escape_sequence) @escape

; Numbers
(integer) @number
(float) @float

; Comments
(comment) @comment

; Boolean constants
(true) @constant.builtin
(false) @constant.builtin

; None constant
(none) @constant.builtin

; Self parameter
((identifier) @variable.builtin
  (#match? @variable.builtin "^self$"))

; Operators
"+" @operator
"-" @operator
"*" @operator
"/" @operator
"//" @operator
"%" @operator
"**" @operator
"=" @operator
"+=" @operator
"-=" @operator
"*=" @operator
"/=" @operator
"//=" @operator
"%=" @operator
"**=" @operator
"&=" @operator
"|=" @operator
"^=" @operator
">>=" @operator
"<<=" @operator
"==" @operator
"!=" @operator
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
"@" @operator

; Punctuation
"(" @punctuation.bracket
")" @punctuation.bracket
"[" @punctuation.bracket
"]" @punctuation.bracket
"{" @punctuation.bracket
"}" @punctuation.bracket
"," @punctuation.delimiter
":" @punctuation.delimiter
"." @punctuation.delimiter
";" @punctuation.delimiter

; Identifiers
(identifier) @variable
)QUERY";

}  // namespace terminal_ui_kit::syntax_queries
