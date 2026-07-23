#include "terminal_ui_kit/markdown/markdown_document.h"

#include <cmark-gfm.h>
#include <cmark-gfm-core-extensions.h>

namespace terminal_ui_kit {

void CmarkNodeDeleter::operator()(cmark_node* node) const {
  cmark_node_free(node);
}

MarkdownDocument::MarkdownDocument(std::string_view markdown) {
  cmark_gfm_core_extensions_ensure_registered();
  root_.reset(cmark_parse_document(
      markdown.data(), markdown.size(), CMARK_OPT_DEFAULT));
}

cmark_node* MarkdownDocument::root() const { return root_.get(); }

}  // namespace terminal_ui_kit