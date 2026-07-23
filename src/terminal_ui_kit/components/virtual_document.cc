#include "terminal_ui_kit/components/virtual_document.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/box.hpp>

#include "terminal_ui_kit/components/style_bridge.h"
#include "terminal_ui_kit/components/virtual_list.h"
#include "terminal_ui_kit/core/selection.h"
#include "terminal_ui_kit/core/styled_text.h"
#include "terminal_ui_kit/document/streaming_document.h"
#include "terminal_ui_kit/document/wrapped_document.h"

namespace terminal_ui_kit {
namespace {

class WidthTracker : public ftxui::Node {
 public:
  WidthTracker(ftxui::Element child, int& observed_width)
      : Node({std::move(child)}), observed_width_(observed_width) {}

  void ComputeRequirement() override {
    children_[0]->ComputeRequirement();
    requirement_ = children_[0]->requirement();
  }

  void SetBox(ftxui::Box box) override {
    observed_width_ = std::max(1, box.x_max - box.x_min + 1);
    Node::SetBox(box);
    children_[0]->SetBox(box);
  }

  void Render(ftxui::Screen& screen) override { children_[0]->Render(screen); }

 private:
  int& observed_width_;
};

}  // namespace

class VirtualDocumentImpl {
 public:
  explicit VirtualDocumentImpl(VirtualDocumentOptions options)
      : options_(std::move(options)),
        wrapped_(WrappedDocument(80, options_.tab_width)),
        last_revision_(0) {
    VirtualListOptions list_opts;
    list_opts.item_count = [this] { return wrapped_.display_line_count(); };
    list_opts.item_height = 1;
    list_opts.render_item = [this](std::size_t index, int width) {
      return render_display_line(index, width);
    };

    model_ = std::make_shared<VirtualListModel>(std::move(list_opts));
    auto list_component = model_->component();

    component_ = ftxui::Renderer(list_component, [this, list_component] {
      check_revision_and_follow();
      update_selection_end();
      return std::make_shared<WidthTracker>(list_component->Render(), current_width_);
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

  void scroll_to_line(std::size_t logical_line_index) {
    if (!options_.document) {
      return;
    }
    check_revision_and_follow();
    const std::size_t display_index = wrapped_.first_display_line_for(logical_line_index);
    model_->scroll_to_index(display_index);
  }

 private:
  void check_revision_and_follow() {
    if (!options_.document) {
      return;
    }
    const std::uint64_t rev = options_.document->revision();
    if (rev == last_revision_ && current_width_ == last_width_) {
      return;
    }
    if (rev != last_revision_) {
      last_revision_ = rev;
      wrapped_.append_from(*options_.document);
    }
    if (current_width_ != last_width_ && current_width_ > 0) {
      last_width_ = current_width_;
      wrapped_.handle_width_change(last_width_, *options_.document);
    }
    if (options_.follow) {
      model_->scroll_to_bottom();
    }
  }

  ftxui::Element render_display_line(std::size_t index, int /*width*/) {
    const WrappedLine& line = wrapped_.display_line_at(index);
    std::string display_text = line.text;

    ftxui::Elements parts;
    if (options_.show_line_numbers) {
      if (line.sub_line == 0) {
        std::string num = std::to_string(line.logical_line + 1);
        num = std::string(5 - num.size(), ' ') + num;
        parts.push_back(ftxui::text(num) | ftxui::color(ftxui::Color::GrayDark));
        parts.push_back(ftxui::text(" "));
      } else {
        parts.push_back(ftxui::text("       "));
      }
    }

    if (selection_.has_selection() && options_.document &&
        line.logical_line < options_.document->line_count()) {
      TextRange sel = selection_.range();
      std::size_t seg_start = line.byte_offset;
      std::size_t seg_end = seg_start + display_text.size();

      if (line.logical_line == sel.start.line &&
          line.logical_line == sel.end.line) {
        // Selection on same logical line - may affect this display segment
        if (sel.end.column <= seg_start || sel.start.column >= seg_end) {
          parts.push_back(ftxui::text(display_text));
        } else {
          std::size_t split_a = (sel.start.column > seg_start)
                                    ? sel.start.column - seg_start
                                    : 0;
          std::size_t split_b = (sel.end.column < seg_end)
                                    ? sel.end.column - seg_start
                                    : display_text.size();
          if (split_a > 0) {
            parts.push_back(ftxui::text(display_text.substr(0, split_a)));
          }
          parts.push_back(
              ftxui::text(display_text.substr(split_a, split_b - split_a)) |
              ftxui::inverted);
          if (split_b < display_text.size()) {
            parts.push_back(ftxui::text(display_text.substr(split_b)));
          }
        }
      } else if (line.logical_line == sel.start.line) {
        // This line contains the start of selection
        if (sel.start.column > seg_start && sel.start.column < seg_end) {
          std::size_t split = sel.start.column - seg_start;
          parts.push_back(ftxui::text(display_text.substr(0, split)));
          parts.push_back(ftxui::text(display_text.substr(split)) |
                          ftxui::inverted);
        } else if (sel.start.column <= seg_start) {
          parts.push_back(ftxui::text(display_text) | ftxui::inverted);
        } else {
          parts.push_back(ftxui::text(display_text));
        }
      } else if (line.logical_line == sel.end.line) {
        // This line contains the end of selection
        if (sel.end.column > seg_start && sel.end.column < seg_end) {
          std::size_t split = sel.end.column - seg_start;
          parts.push_back(ftxui::text(display_text.substr(0, split)) |
                          ftxui::inverted);
          parts.push_back(ftxui::text(display_text.substr(split)));
        } else if (sel.end.column <= seg_start) {
          parts.push_back(ftxui::text(display_text));
        } else {
          parts.push_back(ftxui::text(display_text) | ftxui::inverted);
        }
      } else if (line.logical_line > sel.start.line &&
                 line.logical_line < sel.end.line) {
        parts.push_back(ftxui::text(display_text) | ftxui::inverted);
      } else {
        parts.push_back(ftxui::text(display_text));
      }
    } else {
      parts.push_back(ftxui::text(display_text));
    }
    return ftxui::hbox(std::move(parts));
  }

  bool handle_event(ftxui::Event event) {
    if (event == ftxui::Event::End) {
      options_.follow = true;
      model_->scroll_to_bottom();
      return true;
    }

    if (event.input() == "y") {
      // y — yank (copy) if selection active
      if (selection_.has_selection() && options_.document) {
        std::string text = selection_.selected_text(*options_.document);
        selection_.clear();
        if (options_.on_copy) {
          options_.on_copy(std::move(text));
        }
        return true;
      }
      return false;
    }

    if (event == ftxui::Event::Character('v')) {
      // v — toggle selection anchor at current position
      auto sel = model_->selected_index();
      if (sel) {
        const WrappedLine& line = wrapped_.display_line_at(*sel);
        TextPosition pos{line.logical_line, line.byte_offset};
        selection_column_ = 0;
        if (selection_.is_selecting()) {
          selection_.extend_to(pos);
        } else {
          selection_.start(pos);
        }
      }
      return true;
    }

    if (event == ftxui::Event::ArrowUp || event == ftxui::Event::ArrowDown) {
      options_.follow = false;
      if (selection_.is_selecting()) {
        selection_column_ = 0;
        return false;
      }
      selection_.clear();
      return false;
    }

    if (event == ftxui::Event::ArrowLeft && selection_.is_selecting()) {
      if (selection_column_ > 0) {
        --selection_column_;
      }
      return true;
    }

    if (event == ftxui::Event::ArrowRight && selection_.is_selecting()) {
      auto sel = model_->selected_index();
      if (sel) {
        const WrappedLine& line = wrapped_.display_line_at(*sel);
        if (selection_column_ < line.text.size()) {
          ++selection_column_;
        }
      }
      return true;
    }

    if ((event.is_mouse() && event.mouse().button == ftxui::Mouse::WheelUp) ||
        (event.is_mouse() && event.mouse().button == ftxui::Mouse::WheelDown)) {
      options_.follow = false;
    }

    return false;
  }

  void update_selection_end() {
    if (!selection_.is_selecting() || !options_.document) {
      return;
    }
    auto sel = model_->selected_index();
    if (!sel) {
      return;
    }
    const WrappedLine& line = wrapped_.display_line_at(*sel);
    std::size_t col = line.byte_offset + selection_column_;
    std::size_t line_len = options_.document->line_at(line.logical_line).size();
    col = std::min(col, line_len);
    selection_.extend_to({line.logical_line, col});
  }

  VirtualDocumentOptions options_;
  WrappedDocument wrapped_;
  std::shared_ptr<VirtualListModel> model_;
  ftxui::Component component_;
  std::uint64_t last_revision_;
  int current_width_ = 0;
  int last_width_ = 0;
  SelectionManager selection_;
  std::size_t selection_column_ = 0;
};

VirtualDocument::VirtualDocument(VirtualDocumentOptions options)
    : impl_(std::make_shared<VirtualDocumentImpl>(std::move(options))) {}

ftxui::Component VirtualDocument::component() const { return impl_->component(); }

bool VirtualDocument::follow() const { return impl_->follow(); }

void VirtualDocument::set_follow(bool follow) { impl_->set_follow(follow); }

void VirtualDocument::scroll_to_bottom() { impl_->scroll_to_bottom(); }

void VirtualDocument::scroll_to_line(std::size_t logical_line_index) {
  impl_->scroll_to_line(logical_line_index);
}

}  // namespace terminal_ui_kit