#include "terminal_ui_kit/syntax/syntax_highlighter.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

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
#include "terminal_ui_kit/syntax/syntax_theme.h"
#include "terminal_ui_kit/theme/theme.h"
#include <tree_sitter/api.h>

extern "C" {
TSLanguage* tree_sitter_c();
TSLanguage* tree_sitter_cpp();
TSLanguage* tree_sitter_python();
TSLanguage* tree_sitter_json();
TSLanguage* tree_sitter_bash();
TSLanguage* tree_sitter_rust();
TSLanguage* tree_sitter_javascript();
#if defined(TERMINAL_UI_KIT_HAS_TREE_SITTER_YAML)
TSLanguage* tree_sitter_yaml();
#endif
#if defined(TERMINAL_UI_KIT_HAS_TREE_SITTER_MARKDOWN)
TSLanguage* tree_sitter_markdown();
#endif
#if defined(TERMINAL_UI_KIT_HAS_TREE_SITTER_DIFF)
TSLanguage* tree_sitter_diff();
#endif
}

namespace terminal_ui_kit {
namespace {

struct LanguageInfo {
  TSLanguage* (*language_fn)();
  const char* query;
};

struct CaptureRange {
  uint32_t start;
  uint32_t end;
  TextStyle style;
  std::size_t order;
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
#if defined(TERMINAL_UI_KIT_HAS_TREE_SITTER_YAML)
  map["yaml"] = {tree_sitter_yaml, syntax_queries::kYamlHighlights};
  map["yml"] = {tree_sitter_yaml, syntax_queries::kYamlHighlights};
#endif
#if defined(TERMINAL_UI_KIT_HAS_TREE_SITTER_MARKDOWN)
  map["markdown"] = {tree_sitter_markdown, syntax_queries::kMarkdownHighlights};
  map["md"] = {tree_sitter_markdown, syntax_queries::kMarkdownHighlights};
#endif
#if defined(TERMINAL_UI_KIT_HAS_TREE_SITTER_DIFF)
  map["diff"] = {tree_sitter_diff, syntax_queries::kDiffHighlights};
#endif
  return map;
}

bool is_capture_family(std::string_view capture, std::string_view family) {
  return capture == family || (capture.size() > family.size() && capture.starts_with(family) &&
                               capture[family.size()] == '.');
}

TextStyle style_for_capture(std::string_view capture, const SyntaxTheme& syntax) {
  if (is_capture_family(capture, "keyword") || capture == "import") {
    return syntax.keyword;
  }
  if (is_capture_family(capture, "type")) {
    return syntax.type;
  }
  if (is_capture_family(capture, "function") || capture == "method" || capture == "constructor") {
    return syntax.function;
  }
  if (is_capture_family(capture, "variable") || is_capture_family(capture, "parameter")) {
    return syntax.variable;
  }
  if (is_capture_family(capture, "string") || is_capture_family(capture, "escape")) {
    return syntax.string;
  }
  if (is_capture_family(capture, "number") || is_capture_family(capture, "float")) {
    return syntax.number;
  }
  if (is_capture_family(capture, "constant")) {
    return syntax.constant;
  }
  if (is_capture_family(capture, "property") || is_capture_family(capture, "field")) {
    return syntax.property;
  }
  if (is_capture_family(capture, "namespace")) {
    return syntax.namespace_style;
  }
  if (is_capture_family(capture, "macro") || is_capture_family(capture, "attribute") ||
      is_capture_family(capture, "decorator")) {
    return syntax.macro;
  }
  if (is_capture_family(capture, "comment")) {
    return syntax.comment;
  }
  if (is_capture_family(capture, "operator") || is_capture_family(capture, "punctuation")) {
    return syntax.operator_style;
  }
  return syntax.variable;
}

}  // namespace

bool SyntaxHighlighter::supports_language(std::string_view language) {
  return language_map().find(language) != language_map().end();
}

StyledText SyntaxHighlighter::highlight(std::string_view code, std::string_view language,
                                        const Theme& theme) {
  StyledText result;

  if (code.empty()) {
    result.append(TextSpan{"", theme.primary, std::nullopt});
    return result;
  }

  const auto& map = language_map();
  auto it = map.find(language);
  if (it == map.end()) {
    result.append(TextSpan{std::string(code), theme.primary, std::nullopt});
    return result;
  }

  const LanguageInfo& info = it->second;
  const SyntaxTheme syntax = theme == default_light_theme() ? default_light_syntax_theme(theme)
                                                            : default_dark_syntax_theme(theme);
  TSLanguage* lang = info.language_fn();
  if (!lang) {
    result.append(TextSpan{std::string(code), theme.primary, std::nullopt});
    return result;
  }

  TSParser* parser = ts_parser_new();
  ts_parser_set_language(parser, lang);

  TSTree* tree =
      ts_parser_parse_string(parser, nullptr, code.data(), static_cast<uint32_t>(code.size()));

  if (!tree) {
    ts_parser_delete(parser);
    result.append(TextSpan{std::string(code), theme.primary, std::nullopt});
    return result;
  }

  uint32_t error_offset = 0;
  TSQueryError error_type = TSQueryErrorNone;
  TSQuery* query = ts_query_new(lang, info.query, static_cast<uint32_t>(std::strlen(info.query)),
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

  std::vector<CaptureRange> captures;
  TSQueryMatch match;
  std::size_t capture_order = 0;

  while (ts_query_cursor_next_match(cursor, &match)) {
    for (uint32_t i = 0; i < match.capture_count; ++i) {
      const TSQueryCapture& capture = match.captures[i];
      uint32_t start = ts_node_start_byte(capture.node);
      uint32_t end = ts_node_end_byte(capture.node);

      uint32_t name_len = 0;
      const char* name = ts_query_capture_name_for_id(query, capture.index, &name_len);
      std::string_view capture_name(name, name_len);
      TextStyle style = style_for_capture(capture_name, syntax);
      if (start < end) {
        captures.push_back(CaptureRange{start, end, style, capture_order});
      }
      ++capture_order;
    }
  }

  std::vector<uint32_t> boundaries = {0, static_cast<uint32_t>(code.size())};
  boundaries.reserve(2 + captures.size() * 2);
  for (const CaptureRange& capture : captures) {
    boundaries.push_back(capture.start);
    boundaries.push_back(capture.end);
  }
  std::sort(boundaries.begin(), boundaries.end());
  boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());

  for (std::size_t i = 0; i + 1 < boundaries.size(); ++i) {
    const uint32_t start = boundaries[i];
    const uint32_t end = boundaries[i + 1];
    if (start == end) {
      continue;
    }

    const CaptureRange* best = nullptr;
    for (const CaptureRange& capture : captures) {
      if (capture.start > start || capture.end < end) {
        continue;
      }
      if (best == nullptr || capture.end - capture.start < best->end - best->start ||
          (capture.end - capture.start == best->end - best->start && capture.order < best->order)) {
        best = &capture;
      }
    }

    const TextStyle& style = best == nullptr ? theme.primary : best->style;
    result.append(TextSpan{std::string(code.substr(start, end - start)), style, std::nullopt});
  }

  ts_query_cursor_delete(cursor);
  ts_query_delete(query);
  ts_tree_delete(tree);
  ts_parser_delete(parser);

  return result;
}

}  // namespace terminal_ui_kit
