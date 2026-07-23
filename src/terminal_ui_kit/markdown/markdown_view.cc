#include "terminal_ui_kit/markdown/markdown_view.h"

#include <memory>
#include <string>
#include <vector>

#include <cmark-gfm.h>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/code_view.h"
#include "terminal_ui_kit/components/style_bridge.h"
#include "terminal_ui_kit/markdown/markdown_document.h"
#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {
namespace {

ftxui::Element render_node(cmark_node* node, const Theme& theme,
                           std::function<void(std::string)> on_link);

ftxui::Element render_children(cmark_node* node, const Theme& theme,
                               std::function<void(std::string)> on_link) {
  ftxui::Elements elements;
  for (cmark_node* child = cmark_node_first_child(node); child;
       child = cmark_node_next(child)) {
    elements.push_back(render_node(child, theme, on_link));
  }
  return ftxui::vbox(std::move(elements));
}

ftxui::Elements collect_inline(cmark_node* node, const Theme& theme,
                               std::function<void(std::string)> on_link) {
  ftxui::Elements parts;
  for (cmark_node* child = cmark_node_first_child(node); child;
       child = cmark_node_next(child)) {
    cmark_node_type type = cmark_node_get_type(child);
    const char* literal = cmark_node_get_literal(child);

    switch (type) {
      case CMARK_NODE_TEXT:
        if (literal) parts.push_back(ftxui::text(literal));
        break;
      case CMARK_NODE_SOFTBREAK:
        parts.push_back(ftxui::text(" "));
        break;
      case CMARK_NODE_LINEBREAK:
        parts.push_back(ftxui::text(""));
        break;
      case CMARK_NODE_CODE:
        if (literal) {
          parts.push_back(ftxui::text(literal) | to_decorator(theme.code));
        }
        break;
      case CMARK_NODE_EMPH:
        for (cmark_node* gc = cmark_node_first_child(child); gc;
             gc = cmark_node_next(gc)) {
          const char* t = cmark_node_get_literal(gc);
          if (t) parts.push_back(ftxui::text(t) | ftxui::dim);
        }
        break;
      case CMARK_NODE_STRONG:
        for (cmark_node* gc = cmark_node_first_child(child); gc;
             gc = cmark_node_next(gc)) {
          const char* t = cmark_node_get_literal(gc);
          if (t) parts.push_back(ftxui::text(t) | ftxui::bold);
        }
        break;
      case CMARK_NODE_LINK: {
        const char* url = cmark_node_get_url(child);
        ftxui::Elements link_parts;
        for (cmark_node* gc = cmark_node_first_child(child); gc;
             gc = cmark_node_next(gc)) {
          const char* t = cmark_node_get_literal(gc);
          if (t) link_parts.push_back(ftxui::text(t));
        }
        ftxui::Element link_el = ftxui::hbox(std::move(link_parts));
        if (url) {
          link_el = link_el | ftxui::hyperlink(url);
        }
        parts.push_back(link_el);
        break;
      }
      default:
        // Recurse for unknown inline types
        parts.push_back(render_node(child, theme, on_link));
        break;
    }
  }
  return parts;
}

ftxui::Element render_node(cmark_node* node, const Theme& theme,
                           std::function<void(std::string)> on_link) {
  cmark_node_type type = cmark_node_get_type(node);
  const char* literal = cmark_node_get_literal(node);

  switch (type) {
    case CMARK_NODE_DOCUMENT:
      return render_children(node, theme, on_link);

    case CMARK_NODE_PARAGRAPH:
      return ftxui::hbox(collect_inline(node, theme, on_link));

    case CMARK_NODE_HEADING: {
      int level = cmark_node_get_heading_level(node);
      ftxui::Elements heading_parts = collect_inline(node, theme, on_link);
      ftxui::Decorator style = ftxui::bold;
      if (level <= 2) style = style | ftxui::underlined;
      return ftxui::hbox(std::move(heading_parts)) | style;
    }

    case CMARK_NODE_BLOCK_QUOTE:
      return ftxui::hbox({
        ftxui::text(" ") | ftxui::dim,
        render_children(node, theme, on_link),
      });

    case CMARK_NODE_LIST: {
      ftxui::Elements items;
      int index = 1;
      cmark_list_type list_type = cmark_node_get_list_type(node);
      for (cmark_node* child = cmark_node_first_child(node); child;
           child = cmark_node_next(child)) {
        if (cmark_node_get_type(child) != CMARK_NODE_ITEM) continue;
        std::string bullet = (list_type == CMARK_ORDERED_LIST)
                                 ? std::to_string(index++) + ". "
                                 : "- ";
        items.push_back(ftxui::hbox({
          ftxui::text(bullet),
          render_children(child, theme, on_link),
        }));
      }
      return ftxui::vbox(std::move(items));
    }

    case CMARK_NODE_CODE_BLOCK: {
      std::string code = literal ? literal : "";
      std::string fence_info = cmark_node_get_fence_info(node);
      return CodeView(code, {fence_info, false, theme});
    }

    case CMARK_NODE_THEMATIC_BREAK:
      return ftxui::separator();

    case CMARK_NODE_HTML_BLOCK:
    case CMARK_NODE_HTML_INLINE:
      if (literal) return ftxui::text(literal) | ftxui::dim;
      return ftxui::text("");

    default:
      return render_children(node, theme, on_link);
  }
}

}  // namespace

ftxui::Component MarkdownView(
    std::shared_ptr<MarkdownDocument> document,
    MarkdownViewOptions options) {
  return ftxui::Renderer([document, options] {
    if (!document || !document->root()) {
      return ftxui::text("");
    }
    return render_node(document->root(), options.theme, options.on_link);
  });
}

}  // namespace terminal_ui_kit