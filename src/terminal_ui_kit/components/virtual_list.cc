#include "terminal_ui_kit/components/virtual_list.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <ftxui/component/animation.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/box.hpp>

namespace terminal_ui_kit {
namespace {

class ObservingBoxDecorator : public ftxui::Node {
 public:
  ObservingBoxDecorator(ftxui::Element child, ftxui::Box& observed_box)
      : Node({std::move(child)}), observed_box_(observed_box) {}

  void ComputeRequirement() override {
    children_[0]->ComputeRequirement();
    requirement_ = children_[0]->requirement();
  }

  void SetBox(ftxui::Box box) override {
    const bool box_changed = observed_box_ != box;
    observed_box_ = box;
    Node::SetBox(box);
    children_[0]->SetBox(box);
    if (box_changed) {
      ftxui::animation::RequestAnimationFrame();
    }
  }

  void Render(ftxui::Screen& screen) override { children_[0]->Render(screen); }

 private:
  ftxui::Box& observed_box_;
};

class RowObserver : public ftxui::Node {
 public:
  RowObserver(ftxui::Element child, std::function<void(int)> on_height)
      : Node({std::move(child)}), on_height_(std::move(on_height)) {}

  void ComputeRequirement() override {
    children_[0]->ComputeRequirement();
    requirement_ = children_[0]->requirement();
  }

  void SetBox(ftxui::Box box) override {
    const bool changed = box_ != box;
    Node::SetBox(box);
    children_[0]->SetBox(box);
    if (changed && on_height_) {
      on_height_(std::max(1, box.y_max - box.y_min + 1));
    }
  }

  void Render(ftxui::Screen& screen) override { children_[0]->Render(screen); }

 private:
  std::function<void(int)> on_height_;
};

ftxui::Element observe_box(ftxui::Element child, ftxui::Box& box) {
  return std::make_shared<ObservingBoxDecorator>(std::move(child), box);
}

}  // namespace

class VirtualListImpl : public ftxui::ComponentBase {
 public:
  explicit VirtualListImpl(VirtualListOptions options) : options_(std::move(options)) {
    normalize();
  }

  void scroll_to_index(std::size_t index) {
    normalize();
    scroll_index_ = std::min(index, max_scroll_index());
  }

  void select_index(std::size_t index) { set_selected(index); }

  std::optional<std::size_t> selected_index() const { return selected_index_; }

 private:
  ftxui::Element Render() override {
    const int width = box_width();
    update_layout_metadata(width);
    normalize();
    ensure_prefix_sums(width);

    ftxui::Elements rows;
    const std::size_t end = visible_end(width);
    for (std::size_t index = scroll_index_; index < end; ++index) {
      ftxui::Element row = options_.render_item(index, width);
      row = std::make_shared<RowObserver>(
          std::move(row), [this, index](int height) { update_measured_height(index, height); });
      row = row | ftxui::size(ftxui::HEIGHT, ftxui::GREATER_THAN, height_for(index, width));
      if (selected_index_ && *selected_index_ == index) {
        row = row | ftxui::inverted;
      }
      rows.push_back(std::move(row));
    }
    return observe_box(ftxui::vbox(std::move(rows)) | ftxui::yflex, box_);
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

  int height_for(std::size_t index, int width) {
    if (index < measured_heights_.size() && measured_heights_[index]) {
      return std::max(1, *measured_heights_[index]);
    }
    if (index < estimated_heights_.size() && estimated_heights_[index] > 0) {
      return estimated_heights_[index];
    }
    if (options_.estimate_height) {
      const int estimate = std::max(1, options_.estimate_height(index, width));
      if (index < estimated_heights_.size()) {
        estimated_heights_[index] = estimate;
      }
      return estimate;
    }
    return std::max(1, options_.item_height);
  }

  int item_height() const { return std::max(1, options_.item_height); }

  void update_layout_metadata(int width) {
    const std::size_t count = item_count();
    if (count != current_item_count_) {
      measured_heights_.resize(count);
      estimated_heights_.resize(count);
      current_item_count_ = count;
      prefix_valid_ = false;
    }
    if (width != current_width_) {
      current_width_ = width;
      std::fill(measured_heights_.begin(), measured_heights_.end(), std::nullopt);
      std::fill(estimated_heights_.begin(), estimated_heights_.end(), 0);
      prefix_valid_ = false;
    }
  }

  void ensure_prefix_sums(int width) {
    if (prefix_valid_) {
      return;
    }
    const std::size_t count = item_count();
    prefix_sums_.assign(count + 1, 0);
    for (std::size_t index = 0; index < count; ++index) {
      prefix_sums_[index + 1] = prefix_sums_[index] + height_for(index, width);
    }
    prefix_valid_ = true;
  }

  void update_measured_height(std::size_t index, int height) {
    if (index >= measured_heights_.size()) {
      return;
    }
    height = std::max(1, height);
    if (measured_heights_[index] && *measured_heights_[index] == height) {
      return;
    }
    measured_heights_[index] = height;
    prefix_valid_ = false;
    ftxui::animation::RequestAnimationFrame();
  }

  std::size_t visible_end(int width) {
    const std::size_t count = item_count();
    ensure_prefix_sums(width);
    if (scroll_index_ >= count) {
      return count;
    }
    const int viewport_bottom = prefix_sums_[scroll_index_] + box_height();
    auto begin = prefix_sums_.begin() + static_cast<std::ptrdiff_t>(scroll_index_);
    auto end_it = std::lower_bound(begin, prefix_sums_.end(), viewport_bottom);
    std::size_t end = static_cast<std::size_t>(end_it - prefix_sums_.begin());
    if (end < count) {
      ++end;
    }
    end = std::min(count, std::max(end, scroll_index_ + 1));
    return end;
  }

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
  std::vector<std::optional<int>> measured_heights_;
  std::vector<int> estimated_heights_;
  std::size_t current_item_count_ = 0;
  int current_width_ = 0;
  bool prefix_valid_ = false;
  std::vector<int> prefix_sums_;
  std::size_t scroll_index_ = 0;
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
