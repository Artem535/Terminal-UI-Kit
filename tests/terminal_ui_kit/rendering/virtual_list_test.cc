#include "terminal_ui_kit/components/virtual_list.h"

#include <string>
#include <utility>

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TEST(VirtualListModel, EmptyListHasNoSelection) {
  VirtualListOptions options;
  options.item_count = [] { return std::size_t{0}; };
  options.render_item = [](std::size_t, int) { return ftxui::text("unused"); };

  VirtualListModel model(std::move(options));

  EXPECT_EQ(model.selected_index(), std::nullopt);
  EXPECT_FALSE(model.component()->Focusable());
}

TEST(VirtualListModel, NonEmptyListInitiallySelectsFirstItemWithoutCallback) {
  std::size_t callback_count = 0;
  VirtualListOptions options;
  options.item_count = [] { return std::size_t{3}; };
  options.render_item = [](std::size_t index, int) {
    return ftxui::text(std::to_string(index));
  };
  options.on_select = [&](std::size_t) { ++callback_count; };

  VirtualListModel model(std::move(options));

  EXPECT_EQ(model.selected_index(), std::optional<std::size_t>{0});
  EXPECT_EQ(callback_count, 0U);
  EXPECT_TRUE(model.component()->Focusable());
}

}  // namespace
}  // namespace terminal_ui_kit
