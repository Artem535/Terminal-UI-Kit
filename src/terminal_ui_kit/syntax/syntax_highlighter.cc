#include "terminal_ui_kit/syntax/syntax_highlighter.h"

#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>

#include <tree_sitter/api.h>

#include "terminal_ui_kit/core/styled_text.h"
#include "terminal_ui_kit/syntax/queries/bash_highlights.h"
#include "terminal_ui_kit/syntax/queries/c_highlights.h"
#include "terminal_ui_kit/syntax/queries/cpp_highlights.h"
#include "terminal_ui_kit/syntax/queries/diff_highlights.h"
#include "terminal_ui_kit/syntax/queries/javascript_highlights.h"
#include "terminal_ui_kit/syntax/queries/json_highlights.h"
#include "terminal_ui_kit/syntax/queries/markdown_highlights.h"
#include "terminal_ui_kit/syntax/queries/python_highlights.h"
#include "terminal_ui_kit/syntax/queries/rust_highlights.h"
#include "terminal_ui_kit/syntax/queries/yaml_highlights.h"
#include "terminal_ui_kit/theme/theme.h"

extern "C" {
TSLanguage* tree_sitter_c();
TSLanguage* tree_sitter_cpp();
TSLanguage* tree_sitter_python();
TSLanguage* tree_sitter_json();
TSLanguage* tree_sitter_bash();
TSLanguage* tree_sitter_rust();
TSLanguage* tree_sitter_javascript();
// These may not be available if grammar fetch failed
TSLanguage* tree_sitter_yaml() __attribute__((weak));
TSLanguage* tree_sitter_markdown() __attribute__((weak));
TSLanguage* tree_sitter_diff() __attribute__((weak));
}

namespace terminal_ui_kit {
namespace {

struct LanguageInfo {
  TSLanguage* (*language_fn)();
  const char* query;
};

const std::unordered_map<std::string_view, LanguageInfo>& language_map() {
  static std::unordered_map<std::string_view, LanguageInfo> map = {
      {"c", {tree_sitter_c, syntax_queries::kCHighlights}},
      {"cpp", {tree_sitter_cpp, syntax_queries::kCppHighlights}},
      {"c++", {tree_sitter_cpp, syntax_queries::kCppHighlights}},
      {"python", {tree_sitter_python, syntax_queries::kPythonHighlights}},
      {"py", {tree_sitter_python, syntax_queries::kPythonHighlights}},
      {"json", {tree_sitter_json, syntax_queries::kJsonHighlights}},
      {"bash", {tree_sitter_bash, syntax_queries::kBashHighlights}},
      {"sh", {tree_sitter_bash, syntax_queries::kBashHighlights}},
      {"shell", {tree_sitter_bash, syntax_queries::kBashHighlights}},
      {"rust", {tree_sitter_rust, syntax_queries::kRustHighlights}},
      {"rs", {tree_sitter_rust, syntax_queries::kRustHighlights}},
      {"javascript", {tree_sitter_javascript, syntax_queries::kJavascriptHighlights}},
      {"js", {tree_sitter_javascript, syntax_queries::kJavascriptHighlights}},
      {"typescript", {tree_sitter_javascript, syntax_queries::kJavascriptHighlights}},
      {"ts", {tree_sitter_javascript, syntax_queries::kJavascriptHighlights}},
  };
  // Add optional grammars only if they were linked
  if (tree_sitter_yaml) {
    map["yaml"] = {tree_sitter_yaml, syntax_queries::kYamlHighlights};
    map["yml"] = {tree_sitter_yaml, syntax_queries::kYamlHighlights};
  }
  if (tree_sitter_markdown) {
    map["markdown"] = {tree_sitter_markdown, syntax_queries::kMarkdownHighlights};
    map["md"] = {tree_sitter_markdown, syntax_queries::kMarkdownHighlights};
  }
  if (tree_sitter_diff) {
    map["diff"] = {tree_sitter_diff, syntax_queries::kDiffHighlights};
  }
  return map;
}

TextStyle style_for_capture(std::string_view capture, const Theme& theme) {
  if (capture == "keyword" || capture == "keyword.return" ||
      capture == "keyword.type" || capture == "keyword.operator" ||
      capture == "import") {
    return theme.accent;
  }
  if (capture == "string" || capture == "escape") {
    return theme.success;
  }
  if (capture == "number" || capture == "float") {
    return theme.warning;
  }
  if (capture == "comment") {
    return theme.muted;
  }
  if (capture == "type" || capture == "type.builtin") {
    return theme.code;
  }
  if (capture == "function" || capture == "method" ||
      capture == "function.builtin" || capture == "constructor") {
    return theme.primary;
  }
  if (capture == "variable" || capture == "parameter") {
    return theme.primary;
  }
  if (capture == "operator" || capture == "punctuation" ||
      capture == "punctuation.bracket" || capture == "punctuation.delimiter") {
    return theme.secondary;
  }
  if (capture == "property" || capture == "field") {
    return theme.secondary;
  }
  if (capture == "constant" || capture == "constant.builtin") {
    return theme.warning;
  }
  if (capture == "attribute" || capture == "decorator") {
    return theme.muted;
  }
  if (capture == "tag") {
    return theme.accent;
  }
  if (capture == "label" || capture == "lifetime" || capture == "macro") {
    return theme.accent;
  }
  if (capture == "namespace") {
    return theme.code;
  }
  return theme.primary;
}

}  // namespace

StyledText SyntaxHighlighter::highlight(
    std::string_view code,
    std::string_view language,
    const Theme& theme) {
  StyledText result;

  const auto& map = language_map();
  auto it = map.find(language);
  if (it == map.end()) {
    result.append(TextSpan{std::string(code), theme.primary, std::nullopt});
    return result;
  }

  const LanguageInfo& info = it->second;
  TSLanguage* lang = info.language_fn();
  if (!lang) {
    result.append(TextSpan{std::string(code), theme.primary, std::nullopt});
    return result;
  }

  TSParser* parser = ts_parser_new();
  ts_parser_set_language(parser, lang);

  TSTree* tree = ts_parser_parse_string(
      parser, nullptr, code.data(), static_cast<uint32_t>(code.size()));

  if (!tree) {
    ts_parser_delete(parser);
    result.append(TextSpan{std::string(code), theme.primary, std::nullopt});
    return result;
  }

  uint32_t error_offset = 0;
  TSQueryError error_type = TSQueryErrorNone;
  TSQuery* query = ts_query_new(
      lang, info.query, static_cast<uint32_t>(std::strlen(info.query)),
      &error_offset, &error_type);

  if (!query || error_type != TSQueryErrorNone) {
    ts_query_delete(query);
    ts_tree_delete(tree);
    ts_parser_delete(parser);
    result.append(TextSpan{std::string(code), theme.primary, std::nullopt});
    return result;
  }

  TSQueryCursor* cursor = ts_query_cursor_new();
  ts_query_cursor_exec(cursor, query, ts_tree_root_node(tree));

  TSQueryMatch match;
  uint32_t last_end = 0;

  while (ts_query_cursor_next_match(cursor, &match)) {
    for (uint32_t i = 0; i < match.capture_count; ++i) {
      const TSQueryCapture& capture = match.captures[i];
      uint32_t start = ts_node_start_byte(capture.node);
      uint32_t end = ts_node_end_byte(capture.node);

      if (start > last_end) {
        std::string_view gap = code.substr(last_end, start - last_end);
        result.append(TextSpan{std::string(gap), theme.primary, std::nullopt});
      }

      uint32_t name_len = 0;
      const char* name = ts_query_capture_name_for_id(
          query, capture.index, &name_len);
      std::string_view capture_name(name, name_len);
      TextStyle style = style_for_capture(capture_name, theme);

      std::string_view text = code.substr(start, end - start);
      result.append(TextSpan{std::string(text), style, std::nullopt});

      last_end = end;
    }
  }

  if (last_end < code.size()) {
    std::string_view remaining = code.substr(last_end);
    result.append(TextSpan{std::string(remaining), theme.primary, std::nullopt});
  }

  ts_query_cursor_delete(cursor);
  ts_query_delete(query);
  ts_tree_delete(tree);
  ts_parser_delete(parser);

  return result;
}

}  // namespace terminal_ui_kit