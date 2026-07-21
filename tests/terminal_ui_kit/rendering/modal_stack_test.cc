#include "terminal_ui_kit/components/modal_stack.h"

#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

ftxui::Component MakeToggleComponent(bool* toggled) {
  return ftxui::CatchEvent(ftxui::Renderer([] { return ftxui::text("layer"); }),
                           [toggled](ftxui::Event event) {
                             if (event == ftxui::Event::Character('x')) {
                               *toggled = true;
                               return true;
                             }
                             return false;
                           });
}

TEST(ModalStack, RendersOnlyBaseWhenEmpty) {
  ModalStack stack(ftxui::Renderer([] { return ftxui::text("base content"); }));

  EXPECT_TRUE(stack.empty());
  std::string text = test_support::render_to_text(stack.component()->Render(), 40, 3);
  EXPECT_NE(text.find("base content"), std::string::npos);
}

TEST(ModalStack, PushShowsModalOnTopOfBase) {
  ModalStack stack(ftxui::Renderer([] { return ftxui::text("base content"); }));
  stack.push(ftxui::Renderer([] { return ftxui::text("modal content"); }));

  EXPECT_FALSE(stack.empty());
  std::string text = test_support::render_to_text(stack.component()->Render(), 40, 5);
  EXPECT_NE(text.find("modal content"), std::string::npos);
}

TEST(ModalStack, EventsRouteToTopmostModalOnly) {
  bool base_toggled = false;
  bool modal_toggled = false;

  ModalStack stack(MakeToggleComponent(&base_toggled));
  stack.push(MakeToggleComponent(&modal_toggled));

  stack.component()->OnEvent(ftxui::Event::Character('x'));

  EXPECT_TRUE(modal_toggled);
  EXPECT_FALSE(base_toggled);
}

TEST(ModalStack, EventsRouteToBaseWhenNoModalPushed) {
  bool base_toggled = false;
  ModalStack stack(MakeToggleComponent(&base_toggled));

  stack.component()->OnEvent(ftxui::Event::Character('x'));

  EXPECT_TRUE(base_toggled);
}

TEST(ModalStack, PopRemovesTopmostModal) {
  bool base_toggled = false;
  bool modal_toggled = false;

  ModalStack stack(MakeToggleComponent(&base_toggled));
  stack.push(MakeToggleComponent(&modal_toggled));
  stack.pop();

  EXPECT_TRUE(stack.empty());
  stack.component()->OnEvent(ftxui::Event::Character('x'));
  EXPECT_TRUE(base_toggled);
  EXPECT_FALSE(modal_toggled);
}

TEST(ModalStack, MultipleModalsOnlyTopReceivesEvents) {
  bool first_toggled = false;
  bool second_toggled = false;

  ModalStack stack(ftxui::Renderer([] { return ftxui::text("base"); }));
  stack.push(MakeToggleComponent(&first_toggled));
  stack.push(MakeToggleComponent(&second_toggled));

  stack.component()->OnEvent(ftxui::Event::Character('x'));

  EXPECT_TRUE(second_toggled);
  EXPECT_FALSE(first_toggled);
}

}  // namespace
}  // namespace terminal_ui_kit
