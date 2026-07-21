#include "terminal_ui_kit/components/virtual_list.h"

#include <utility>

#include <ftxui/component/component_base.hpp>

namespace terminal_ui_kit {

class VirtualListImpl : public ftxui::ComponentBase {
 public:
  explicit VirtualListImpl(VirtualListOptions options) : options_(std::move(options)) {
    normalize();
  }

  void scroll_to_index(std::size_t) {}
  void select_index(std::size_t) {}
  std::optional<std::size_t> selected_index() const { return selected_index_; }

 private:
  ftxui::Element Render() override { return ftxui::text(""); }
  bool Focusable() const override { return selected_index_.has_value(); }
  void normalize() {
    if (!options_.item_count || options_.item_count() == 0) {
      selected_index_.reset();
      return;
    }
    selected_index_ = 0;
  }

  VirtualListOptions options_;
  std::optional<std::size_t> selected_index_;
};

ftxui::Component VirtualList(VirtualListOptions options) {
  return ftxui::Make<VirtualListImpl>(std::move(options));
}

VirtualListModel::VirtualListModel(VirtualListOptions options)
    : impl_(ftxui::Make<VirtualListImpl>(std::move(options))) {}

ftxui::Component VirtualListModel::component() const { return impl_; }

void VirtualListModel::scroll_to_index(std::size_t index) { impl_->scroll_to_index(index); }

void VirtualListModel::select_index(std::size_t index) { impl_->select_index(index); }

std::optional<std::size_t> VirtualListModel::selected_index() const {
  return impl_->selected_index();
}

}  // namespace terminal_ui_kit
