#pragma once

#include <memory>

#include <ftxui/component/component.hpp>

namespace terminal_ui_kit {

// How the base layer is treated while a modal is open.
enum class BackdropStyle {
  kNone,
  kClear,
  kDim,
};

class ModalStackImpl;

// A stack of nested modal layers over a base component. Only the topmost
// modal receives events while the stack is non-empty.
class ModalStack {
 public:
  explicit ModalStack(ftxui::Component base, BackdropStyle style = BackdropStyle::kClear);

  ftxui::Component component() const;

  void push(ftxui::Component modal);
  void pop();
  bool empty() const;

 private:
  std::shared_ptr<ModalStackImpl> impl_;
};

}  // namespace terminal_ui_kit
