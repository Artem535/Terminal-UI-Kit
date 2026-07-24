#pragma once

namespace terminal_ui_kit::syntax_queries {

constexpr const char* kRustHighlights = R"QUERY(
; Keywords
"if" @keyword
"else" @keyword
"match" @keyword
"for" @keyword
"while" @keyword
"loop" @keyword
"break" @keyword
"continue" @keyword
"return" @keyword.return
"yield" @keyword
"async" @keyword
"await" @keyword

; Type keywords
"struct" @keyword.type
"enum" @keyword.type
"union" @keyword.type
"type" @keyword.type
"trait" @keyword.type
"impl" @keyword.type
"where" @keyword.type
"dyn" @keyword.type

; Visibility
"pub" @keyword
(self) @keyword
(super) @keyword
(crate) @keyword
(mutable_specifier) @keyword

; Storage
"let" @keyword
"const" @keyword
"static" @keyword
"ref" @keyword
"move" @keyword
"unsafe" @keyword
"extern" @keyword
"fn" @keyword

; Use statements
"use" @import
"mod" @import

(use_declaration
  argument: (scoped_identifier
    path: (identifier) @namespace))

(use_declaration
  argument: (identifier) @namespace)

; Modules
(mod_item
  name: (identifier) @namespace)

; Function definitions
(function_item
  name: (identifier) @function)

; Method definitions
(impl_item
  body: (declaration_list
    (function_item
      name: (identifier) @method)))

; Function calls
(call_expression
  function: (identifier) @function)

(call_expression
  function: (field_expression
    field: (field_identifier) @method))

(call_expression
  function: (scoped_identifier
    name: (identifier) @function))

; Macro calls
(macro_invocation
  macro: (identifier) @macro)

(macro_definition
  name: (identifier) @macro)

; Attributes
(attribute_item) @attribute
(inner_attribute_item) @attribute

; Parameters
(parameter
  pattern: (identifier) @parameter)

(ref_pattern
  (identifier) @parameter)

(mut_pattern
  (identifier) @parameter)

(closure_parameters
  (identifier) @parameter)

; Struct fields
(field_identifier) @field

; Type identifiers
(type_identifier) @type

; Labels
(label) @label

; Primitive types
((primitive_type) @type.builtin
  (#match? @type.builtin "^(bool|char|f32|f64|i8|i16|i32|i64|i128|isize|str|u8|u16|u32|u64|u128|usize)$"))

; Strings
(string_literal) @string
(raw_string_literal) @string
(char_literal) @string

; Escape sequences
(escape_sequence) @escape

; Numbers
(integer_literal) @number
(float_literal) @float

; Comments
(line_comment) @comment
(block_comment) @comment

; Lifetimes
(lifetime) @lifetime

; Boolean constants
"true" @constant.builtin
"false" @constant.builtin

; Operators
"+" @operator
"-" @operator
"*" @operator
"/" @operator
"%" @operator
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
"<<" @operator
">>" @operator
"?" @operator
".." @operator
".." @operator
"=>" @operator
"->" @operator

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
"::" @punctuation.delimiter

; Identifiers
(identifier) @variable
)QUERY";

}  // namespace terminal_ui_kit::syntax_queries
