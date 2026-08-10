#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "terminal_ui_kit/core/text_position.h"

namespace terminal_ui_kit {

// Terminal-agnostic multiline text-buffer model (PRD section 21).
//
// EditorDocument owns and mutates a vector of lines plus a logical cursor. It
// is deliberately free of any FTXUI / terminal dependency so it can be unit
// tested and reused by non-terminal consumers. The FTXUI view
// (MultilineEditor) reads this model and calls its editing/navigation methods.
//
// Conventions:
//  * The document always contains at least one line (an empty buffer is a
//    single "" line).
//  * The cursor column is a byte offset into a line and is always at a UTF-8
//    code-point boundary, so line.substr(cursor.column) never splits a
//    multi-byte character.
//  * Editing, navigation and viewport resizes keep the cursor inside the
//    viewport (invariant: viewport contains cursor after an action).
//  * A "preferred column" is preserved across vertical movement so moving
//    up/down across lines of differing length stays aligned (deterministic).
class EditorDocument {
 public:
  EditorDocument();
  explicit EditorDocument(std::vector<std::string> lines);

  EditorDocument(const EditorDocument&) = default;
  EditorDocument& operator=(const EditorDocument&) = default;
  EditorDocument(EditorDocument&&) = default;
  EditorDocument& operator=(EditorDocument&&) = default;

  // --- Text access ---------------------------------------------------------
  // Number of lines. Always >= 1.
  std::size_t line_count() const;
  // Line at |index| (safe: out-of-range read is clamped to the last line).
  const std::string& line(std::size_t index) const;
  const std::vector<std::string>& lines() const;
  // Whole document joined with '\n' (no trailing newline).
  std::string text() const;

  // --- Cursor --------------------------------------------------------------
  TextPosition cursor() const;
  std::size_t cursor_line() const;
  // Byte offset into the current line (always a UTF-8 code-point boundary).
  std::size_t cursor_column() const;
  void set_cursor(TextPosition position);

  // --- Replacement ---------------------------------------------------------
  // Replace all content. Cursor is reset to the start of the document.
  void set_text(std::string text);
  void set_lines(std::vector<std::string> lines);
  void clear();

  // --- Editing -------------------------------------------------------------
  // Insert |text| at the cursor. A '\n' inside |text| splits lines, so a
  // multi-line paste is inserted efficiently (no per-character quadratic
  // growth). The cursor ends after the inserted text.
  void insert_text(std::string_view text);
  void insert_char(char c);
  // Split the current line at the cursor; the cursor moves to the start of the
  // new line.
  void insert_newline();
  void delete_backward();
  void delete_forward();

  // --- Navigation ----------------------------------------------------------
  void move_left();
  void move_right();
  void move_up();
  void move_down();
  void move_home();
  void move_end();
  void move_word_left();
  void move_word_right();

  // --- Viewport ------------------------------------------------------------
  // Set the visible size in cells (columns x rows). Triggers scroll so the
  // cursor stays visible (handles terminal resize).
  void set_viewport_size(std::size_t width, std::size_t height);
  std::size_t viewport_width() const;
  std::size_t viewport_height() const;
  // Index of the first visible line.
  std::size_t scroll_top() const;
  // Byte offset of the first visible column (a code-point boundary).
  std::size_t scroll_left() const;
  // Scroll the viewport so the cursor is visible.
  void scroll_to_cursor();

  // --- Misc ----------------------------------------------------------------
  // Byte offset remembered for vertical movement (always a code-point
  // boundary; clamped per line when applied).
  std::size_t preferred_column() const;
  // Incremented on every mutation; lets observers cheaply detect edits.
  std::uint64_t revision() const;

 private:
  // Clamp cursor, preferred column and viewport to the current line layout.
  void normalize();
  void ensure_cursor_visible();
  void update_preferred_column();
  // Set cursor column from the preferred column clamped to the current line.
  void apply_preferred_column();
  // Largest byte offset <= |column| that is a UTF-8 code-point start in
  // |line| (rounds a mid-code-point offset down).
  static std::size_t clamp_to_codepoint(const std::string& line, std::size_t column);
  // Byte offset, in |line|, of the code point immediately before |column|.
  static std::size_t previous_code_point(const std::string& line, std::size_t column);
  // Byte offset, in |line|, of the code point immediately after |column|.
  static std::size_t next_code_point(const std::string& line, std::size_t column);
  // Smallest byte offset >= |column| that is a code-point start in |line|.
  static std::size_t round_up_to_code_point(const std::string& line, std::size_t column);

  std::vector<std::string> lines_{""};
  TextPosition cursor_{0, 0};
  std::size_t preferred_column_ = 0;
  std::size_t viewport_width_ = 80;
  std::size_t viewport_height_ = 10;
  std::size_t scroll_top_ = 0;
  std::size_t scroll_left_ = 0;
  std::uint64_t revision_ = 0;
};

}  // namespace terminal_ui_kit
