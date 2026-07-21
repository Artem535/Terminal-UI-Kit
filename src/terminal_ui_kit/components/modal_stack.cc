#include "terminal_ui_kit/components/modal_stack.h"

#include <utility>
#include <vector>

#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

namespace terminal_ui_kit {

class ModalStackImpl : public ftxui::ComponentBase {
 public:
  ModalStackImpl(ftxui::Component base, BackdropStyle style)
      : base_(std::move(base)), style_(style) {
    Add(base_);
  }

  void Push(ftxui::Component modal) {
    Add(modal);
    modals_.push_back(std::move(modal));
  }

  void Pop() {
    if (modals_.empty()) {
      return;
    }
    modals_.back()->Detach();
    modals_.pop_back();
  }

  bool Empty() const { return modals_.empty(); }

 private:
  ftxui::Element Render() override {
    ftxui::Element base_element = base_->Render();
    if (style_ == BackdropStyle::kDim && !modals_.empty()) {
      base_element = base_element | ftxui::dim;
    }

    ftxui::Elements layers = {base_element};
    for (ftxui::Component& modal : modals_) {
      ftxui::Element modal_element = modal->Render();
      if (style_ != BackdropStyle::kNone) {
        modal_element = modal_element | ftxui::clear_under;
      }
      modal_element = modal_element | ftxui::center;
      layers.push_back(std::move(modal_element));
    }
    return ftxui::dbox(std::move(layers));
  }

  bool OnEvent(ftxui::Event event) override {
    if (!modals_.empty()) {
      return modals_.back()->OnEvent(event);
    }
    return base_->OnEvent(event);
  }

  ftxui::Component ActiveChild() override {
    if (!modals_.empty()) {
      return modals_.back();
    }
    return base_;
  }

  ftxui::Component base_;
  BackdropStyle style_;
  std::vector<ftxui::Component> modals_;
};

ModalStack::ModalStack(ftxui::Component base, BackdropStyle style)
    : impl_(ftxui::Make<ModalStackImpl>(std::move(base), style)) {}

ftxui::Component ModalStack::component() const { return impl_; }

void ModalStack::push(ftxui::Component modal) { impl_->Push(std::move(modal)); }

void ModalStack::pop() { impl_->Pop(); }

bool ModalStack::empty() const { return impl_->Empty(); }

}  // namespace terminal_ui_kit
