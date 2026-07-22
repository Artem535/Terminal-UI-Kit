#include "terminal_ui_kit/components/log_view.h"

#include <cstddef>
#include <string>
#include <utility>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/style_bridge.h"
#include "terminal_ui_kit/core/styled_text.h"
#include "terminal_ui_kit/document/log_model.h"
#include "terminal_ui_kit/document/streaming_document.h"
#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

namespace {

ftxui::Element severity_badge(LogSeverity severity, const Theme& theme) {
  std::string label;
  TextStyle badge_style;
  switch (severity) {
    case LogSeverity::kTrace:
      label = " TRCE ";
      badge_style = theme.muted;
      break;
    case LogSeverity::kDebug:
      label = " DEBUG";
      badge_style = theme.code;
      break;
    case LogSeverity::kInfo:
      label = " INFO ";
      badge_style = theme.accent;
      break;
    case LogSeverity::kWarning:
      label = " WARN ";
      badge_style = theme.warning;
      break;
    case LogSeverity::kError:
      label = " ERR ";
      badge_style = theme.error;
      break;
  }
  return ftxui::text(label) | to_decorator(badge_style) | ftxui::dim;
}

}  // namespace

class LogViewImpl {
 public:
  explicit LogViewImpl(LogViewOptions options) : options_(std::move(options)), last_revision_(0) {
    VirtualListOptions list_opts;
    list_opts.item_count = [this] { return source_size(); };
    list_opts.render_item = [this](std::size_t index, int) { return render_line(index); };

    model_ = std::make_shared<VirtualListModel>(std::move(list_opts));
    auto list_component = model_->component();

    component_ = ftxui::Renderer(list_component, [this, list_component] {
      check_revision_and_follow();
      return list_component->Render();
    });
    component_ |= ftxui::CatchEvent([this](ftxui::Event event) { return handle_event(event); });
  }

  ftxui::Component component() const { return component_; }

  bool follow() const { return options_.follow; }
  void set_follow(bool follow) {
    options_.follow = follow;
    if (follow) {
      model_->scroll_to_bottom();
    }
  }
  void scroll_to_bottom() { model_->scroll_to_bottom(); }

 private:
  std::size_t source_size() const {
    if (options_.document) return options_.document->line_count();
    if (options_.log_model) return options_.log_model->size();
    return 0;
  }

  void check_revision_and_follow() {
    std::uint64_t rev = 0;
    if (options_.document) {
      rev = options_.document->revision();
    } else if (options_.log_model) {
      rev = options_.log_model->revision();
    }
    if (options_.follow && rev != last_revision_) {
      model_->scroll_to_bottom();
      last_revision_ = rev;
    }
  }

  ftxui::Element render_line(std::size_t index) {
    if (options_.document) {
      return ftxui::text(std::string(options_.document->line_at(index)));
    }
    if (options_.log_model) {
      const LogEntry& entry = options_.log_model->at(index);
      ftxui::Elements parts;
      if (options_.show_severity) {
        parts.push_back(severity_badge(entry.severity, options_.theme));
        parts.push_back(ftxui::text(" "));
      }
      if (options_.show_timestamp && !entry.timestamp.empty()) {
        parts.push_back(ftxui::text(entry.timestamp) | to_decorator(options_.theme.muted));
        parts.push_back(ftxui::text(" "));
      }
      parts.push_back(render_styled_text(entry.message));
      return ftxui::hbox(std::move(parts));
    }
    return ftxui::text("");
  }

  bool handle_event(ftxui::Event event) {
    if (event == ftxui::Event::ArrowUp || event == ftxui::Event::ArrowDown ||
        (event.is_mouse() && event.mouse().button == ftxui::Mouse::WheelUp) ||
        (event.is_mouse() && event.mouse().button == ftxui::Mouse::WheelDown)) {
      options_.follow = false;
    }
    if (event == ftxui::Event::End) {
      options_.follow = true;
      model_->scroll_to_bottom();
      return true;
    }
    return false;
  }

  LogViewOptions options_;
  std::shared_ptr<VirtualListModel> model_;
  ftxui::Component component_;
  std::uint64_t last_revision_ = 0;
};

LogView::LogView(LogViewOptions options)
    : impl_(std::make_shared<LogViewImpl>(std::move(options))) {}

ftxui::Component LogView::component() const { return impl_->component(); }
bool LogView::follow() const { return impl_->follow(); }
void LogView::set_follow(bool follow) { impl_->set_follow(follow); }
void LogView::scroll_to_bottom() { impl_->scroll_to_bottom(); }

}  // namespace terminal_ui_kit