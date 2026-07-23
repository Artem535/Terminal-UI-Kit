#pragma once

#include <memory>
#include <string_view>

struct cmark_node;

namespace terminal_ui_kit {

struct CmarkNodeDeleter {
  void operator()(cmark_node* node) const;
};

using CmarkNodePtr = std::unique_ptr<cmark_node, CmarkNodeDeleter>;

class MarkdownDocument {
 public:
  explicit MarkdownDocument(std::string_view markdown);
  ~MarkdownDocument() = default;

  MarkdownDocument(MarkdownDocument&&) = default;
  MarkdownDocument& operator=(MarkdownDocument&&) = default;

  [[nodiscard]] cmark_node* root() const;

 private:
  CmarkNodePtr root_;
};

}  // namespace terminal_ui_kit