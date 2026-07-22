#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

namespace terminal_ui_kit {

struct VirtualListOptions {
  std::function<std::size_t()> item_count;
  std::function<ftxui::Element(std::size_t index, int width)> render_item;
  int item_height = 1;
  std::function<void(std::size_t index)> on_select;
  // Optional variable-height estimate. Kept after the PR5 fields so the
  // legacy positional aggregate form remains source-compatible.
  std::function<int(std::size_t index, int width)> estimate_height;
};

class VirtualListImpl;

class VirtualListModel {
 public:
  explicit VirtualListModel(VirtualListOptions options);

  ftxui::Component component() const;
  void scroll_to_index(std::size_t index);
  void scroll_to_bottom();
  void select_index(std::size_t index);
  std::optional<std::size_t> selected_index() const;

 private:
  std::shared_ptr<VirtualListImpl> impl_;
};

ftxui::Component VirtualList(VirtualListOptions options);

}  // namespace terminal_ui_kit
