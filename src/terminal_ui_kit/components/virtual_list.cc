#include "terminal_ui_kit/components/virtual_list.h"

#include <algorithm>
#include <utility>

#include <ftxui/component/animation.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/screen/box.hpp>

namespace terminal_ui_kit {

class VirtualListImpl : public ftxui::ComponentBase {
 public:
  explicit VirtualListImpl(VirtualListOptions options) : options_(std::move(options)) {
    normalize();
  }

  void scroll_to_index(std::size_t index) { scroll_index_ = std::min(index, max_scroll_index()); }

  void select_index(std::size_t index) { set_selected(index); }

  std::optional<std::size_t> selected_index() const { return selected_index_; }

 private:
  ftxui::Element Render() override {
    normalize();
    const int width = box_width();
    const int height = box_height();
    if (width != rendered_width_ || height != rendered_height_) {
      rendered_width_ = width;
      rendered_height_ = height;
      ftxui::animation::RequestAnimationFrame();
    }

    ftxui::Elements rows;
    const std::size_t end = std::min(item_count(), scroll_index_ + viewport_rows());
    for (std::size_t index = scroll_index_; index < end; ++index) {
      ftxui::Element row = options_.render_item(index, width) |
                           ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, item_height());
      if (selected_index_ && *selected_index_ == index) {
        row = row | ftxui::inverted;
      }
      rows.push_back(std::move(row));
    }
    return ftxui::vbox(std::move(rows)) | ftxui::yflex | ftxui::reflect(box_);
  }

  bool Focusable() const override { return selected_index_.has_value(); }

  bool OnEvent(ftxui::Event event) override {
    normalize();
    if (event.is_mouse()) {
      return on_mouse_event(event);
    }

    const std::size_t count = item_count();
    if (count == 0 || !selected_index_) {
      return false;
    }

    const std::size_t selected = *selected_index_;
    if (event == ftxui::Event::ArrowUp && selected > 0) {
      return set_selected(selected - 1);
    }
    if (event == ftxui::Event::ArrowDown && selected + 1 < count) {
      return set_selected(selected + 1);
    }
    if (event == ftxui::Event::PageUp && selected > 0) {
      return set_selected(selected > viewport_rows() ? selected - viewport_rows() : 0);
    }
    if (event == ftxui::Event::PageDown && selected + 1 < count) {
      return set_selected(std::min(count - 1, selected + viewport_rows()));
    }
    if (event == ftxui::Event::Home) {
      return set_selected(0);
    }
    if (event == ftxui::Event::End) {
      return set_selected(count - 1);
    }
    return false;
  }

  std::size_t item_count() const { return options_.item_count ? options_.item_count() : 0; }

  int item_height() const { return std::max(1, options_.item_height); }

  std::size_t viewport_rows() const {
    return static_cast<std::size_t>((box_height() + item_height() - 1) / item_height());
  }

  void normalize() {
    const std::size_t count = item_count();
    if (count == 0) {
      scroll_index_ = 0;
      selected_index_.reset();
      return;
    }
    if (!selected_index_) {
      selected_index_ = 0;
    }
    *selected_index_ = std::min(*selected_index_, count - 1);
    scroll_index_ = std::min(scroll_index_, max_scroll_index());
  }

  void ensure_visible(std::size_t index) {
    const std::size_t rows = viewport_rows();
    if (index < scroll_index_) {
      scroll_index_ = index;
    } else if (index >= scroll_index_ + rows) {
      scroll_index_ = index - rows + 1;
    }
  }

  bool set_selected(std::size_t index) {
    normalize();
    const std::size_t count = item_count();
    if (count == 0) {
      return false;
    }
    index = std::min(index, count - 1);
    if (selected_index_ && *selected_index_ == index) {
      return false;
    }
    selected_index_ = index;
    ensure_visible(index);
    if (options_.on_select) {
      options_.on_select(index);
    }
    return true;
  }

  bool on_mouse_event(ftxui::Event& event) {
    const ftxui::Mouse& mouse = event.mouse();
    if (!box_.Contain(mouse.x, mouse.y)) {
      return false;
    }

    const std::size_t previous_scroll_index = scroll_index_;
    if (mouse.button == ftxui::Mouse::WheelUp) {
      scroll_index_ = scroll_index_ > 3 ? scroll_index_ - 3 : 0;
    } else if (mouse.button == ftxui::Mouse::WheelDown) {
      scroll_index_ = std::min(scroll_index_ + 3, max_scroll_index());
    } else {
      return false;
    }
    return scroll_index_ != previous_scroll_index;
  }

  std::size_t max_scroll_index() const {
    const std::size_t count = item_count();
    const std::size_t rows = viewport_rows();
    return count > rows ? count - rows : 0;
  }

  int box_width() const { return std::max(1, box_.x_max - box_.x_min + 1); }

  int box_height() const { return std::max(1, box_.y_max - box_.y_min + 1); }

  VirtualListOptions options_;
  ftxui::Box box_;
  std::size_t scroll_index_ = 0;
  std::optional<std::size_t> selected_index_;
  int rendered_width_ = 0;
  int rendered_height_ = 0;
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
