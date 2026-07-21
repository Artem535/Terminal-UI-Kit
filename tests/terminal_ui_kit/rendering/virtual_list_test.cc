#include "terminal_ui_kit/components/virtual_list.h"

#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>

#include "terminal_ui_kit/testing/virtual_screen.h"
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
  options.render_item = [](std::size_t index, int) { return ftxui::text(std::to_string(index)); };
  options.on_select = [&](std::size_t) { ++callback_count; };

  VirtualListModel model(std::move(options));

  EXPECT_EQ(model.selected_index(), std::optional<std::size_t>{0});
  EXPECT_EQ(callback_count, 0U);
  EXPECT_TRUE(model.component()->Focusable());
}

TEST(VirtualList, RendersOnlyViewportItemsFromLargeList) {
  std::size_t render_count = 0;
  VirtualListOptions options;
  options.item_count = [] { return std::size_t{100000}; };
  options.render_item = [&render_count](std::size_t index, int) {
    ++render_count;
    return ftxui::text(std::to_string(index));
  };

  VirtualListModel model(std::move(options));
  test_support::render_to_screen(model.component()->Render(), 20, 4);
  render_count = 0;
  test_support::render_to_screen(model.component()->Render(), 20, 4);

  EXPECT_GT(render_count, 0U);
  EXPECT_LE(render_count, 4U);
}

TEST(VirtualList, ArrowDownChangesSelectionAndInvokesCallback) {
  std::vector<std::size_t> selections;
  VirtualListOptions options;
  options.item_count = [] { return std::size_t{3}; };
  options.render_item = [](std::size_t index, int) { return ftxui::text(std::to_string(index)); };
  options.on_select = [&selections](std::size_t index) { selections.push_back(index); };
  VirtualListModel model(std::move(options));

  EXPECT_TRUE(model.component()->OnEvent(ftxui::Event::ArrowDown));

  EXPECT_EQ(model.selected_index(), std::optional<std::size_t>{1});
  EXPECT_EQ(selections, std::vector<std::size_t>{1});
}

TEST(VirtualList, SelectedItemIsInverted) {
  VirtualListOptions options;
  options.item_count = [] { return std::size_t{2}; };
  options.render_item = [](std::size_t index, int) {
    return ftxui::text(index == 0 ? "first" : "second");
  };
  VirtualListModel model(std::move(options));
  test_support::render_to_screen(model.component()->Render(), 20, 2);
  model.select_index(1);

  ftxui::Screen screen = test_support::render_to_screen(model.component()->Render(), 20, 2);
  EXPECT_FALSE(screen.PixelAt(0, 0).inverted);
  EXPECT_TRUE(screen.PixelAt(0, 1).inverted);
}

TEST(VirtualListModel, ModelControlClampsAndPreservesCallbackRules) {
  std::size_t count = 8;
  std::vector<std::size_t> selections;
  VirtualListOptions options;
  options.item_count = [&count] { return count; };
  options.render_item = [](std::size_t index, int) { return ftxui::text(std::to_string(index)); };
  options.on_select = [&selections](std::size_t index) { selections.push_back(index); };
  VirtualListModel model(std::move(options));

  model.select_index(99);
  model.scroll_to_index(99);
  count = 3;
  test_support::render_to_screen(model.component()->Render(), 20, 2);

  EXPECT_EQ(model.selected_index(), std::optional<std::size_t>{2});
  EXPECT_EQ(selections, std::vector<std::size_t>{7});
}

TEST(VirtualList, WheelDownScrollsViewportWithoutChangingSelection) {
  std::vector<std::size_t> rendered_indices;
  VirtualListOptions options;
  options.item_count = [] { return std::size_t{10}; };
  options.render_item = [&rendered_indices](std::size_t index, int) {
    rendered_indices.push_back(index);
    return ftxui::text(std::to_string(index));
  };
  VirtualListModel model(std::move(options));
  test_support::render_to_screen(model.component()->Render(), 20, 4);
  rendered_indices.clear();

  ftxui::Mouse mouse;
  mouse.x = 0;
  mouse.y = 0;
  mouse.button = ftxui::Mouse::WheelDown;
  EXPECT_TRUE(model.component()->OnEvent(ftxui::Event::Mouse("", mouse)));

  test_support::render_to_screen(model.component()->Render(), 20, 4);
  ASSERT_FALSE(rendered_indices.empty());
  EXPECT_EQ(rendered_indices.front(), 3U);
  EXPECT_EQ(model.selected_index(), std::optional<std::size_t>{0});
}

TEST(VirtualList, UsesReflectedWidthOnFrameAfterResize) {
  int last_width = 0;
  VirtualListOptions options;
  options.item_count = [] { return std::size_t{1}; };
  options.render_item = [&last_width](std::size_t, int width) {
    last_width = width;
    return ftxui::text("item");
  };
  VirtualListModel model(std::move(options));

  test_support::render_to_screen(model.component()->Render(), 20, 1);
  test_support::render_to_screen(model.component()->Render(), 40, 1);
  test_support::render_to_screen(model.component()->Render(), 40, 1);

  EXPECT_EQ(last_width, 40);
}

}  // namespace
}  // namespace terminal_ui_kit
