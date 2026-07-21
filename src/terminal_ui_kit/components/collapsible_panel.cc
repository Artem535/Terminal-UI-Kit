#include "terminal_ui_kit/components/collapsible_panel.h"

#include <memory>
#include <utility>

#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

#include "terminal_ui_kit/components/status_indicator.h"
#include "terminal_ui_kit/components/style_bridge.h"

namespace terminal_ui_kit {
namespace {

class CollapsiblePanelHeader : public ftxui::ComponentBase {
 public:
  CollapsiblePanelHeader(std::shared_ptr<bool> expanded, CollapsiblePanelOptions options,
                         Theme theme)
      : expanded_(std::move(expanded)), options_(std::move(options)), theme_(std::move(theme)) {}

 private:
  ftxui::Element Render() override {
    ftxui::Elements row;
    row.push_back(ftxui::text(*expanded_ ? "▾" : "▸") | to_decorator(theme_.muted));
    row.push_back(ftxui::text(" "));
    if (options_.status) {
      row.push_back(StatusIndicator(*options_.status, "", theme_));
      row.push_back(ftxui::text(" "));
    }
    row.push_back(ftxui::text(options_.title) | to_decorator(theme_.primary));
    if (!options_.summary.empty()) {
      row.push_back(ftxui::text("  " + options_.summary) | to_decorator(theme_.secondary));
    }
    row.push_back(ftxui::filler());
    if (!options_.duration_text.empty()) {
      row.push_back(ftxui::text(options_.duration_text) | to_decorator(theme_.muted));
    }

    ftxui::Element element = ftxui::hbox(std::move(row));
    if (Focused()) {
      element = element | ftxui::focus;
    }
    return element | ftxui::reflect(box_);
  }

  bool OnEvent(ftxui::Event event) override {
    if (event.is_mouse()) {
      return OnMouseEvent(event);
    }
    if (event == ftxui::Event::Character(' ') || event == ftxui::Event::Return) {
      *expanded_ = !*expanded_;
      TakeFocus();
      return true;
    }
    return false;
  }

  bool OnMouseEvent(ftxui::Event event) {
    if (!box_.Contain(event.mouse().x, event.mouse().y) || !CaptureMouse(event)) {
      return false;
    }
    if (event.mouse().button == ftxui::Mouse::Left &&
        event.mouse().motion == ftxui::Mouse::Pressed) {
      *expanded_ = !*expanded_;
      TakeFocus();
      return true;
    }
    return false;
  }

  bool Focusable() const override { return true; }

  std::shared_ptr<bool> expanded_;
  CollapsiblePanelOptions options_;
  Theme theme_;
  ftxui::Box box_;
};

}  // namespace

ftxui::Component CollapsiblePanel(CollapsiblePanelOptions options, ftxui::Component body,
                                  const Theme& theme) {
  auto expanded = std::make_shared<bool>(options.initially_expanded);
  ftxui::Component header = ftxui::Make<CollapsiblePanelHeader>(expanded, options, theme);
  ftxui::Component maybe_body = ftxui::Maybe(std::move(body), expanded.get());
  return ftxui::Container::Vertical({std::move(header), std::move(maybe_body)});
}

}  // namespace terminal_ui_kit
