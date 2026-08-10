#include "terminal_ui_kit/editor/editor_document.h"

#include <cstddef>
#include <random>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

EditorDocument DocumentFrom(std::vector<std::string> lines) {
  return EditorDocument(std::move(lines));
}

bool IsCodePointBoundary(const std::string& line, std::size_t column) {
  if (column == 0 || column == line.size()) {
    return true;
  }
  // A code-point boundary never starts on a UTF-8 continuation byte.
  const unsigned char c = static_cast<unsigned char>(line[column]);
  return (c & 0xC0U) != 0x80U;
}

// Structural invariants that must hold after every operation.
void ExpectInvariants(const EditorDocument& doc) {
  EXPECT_GE(doc.line_count(), 1U);
  const std::size_t line = doc.cursor().line;
  EXPECT_LT(line, doc.line_count());
  const std::string& current = doc.line(line);
  EXPECT_LE(doc.cursor().column, current.size());
  EXPECT_TRUE(IsCodePointBoundary(current, doc.cursor().column));
  // The viewport must contain the cursor.
  EXPECT_LE(doc.scroll_top(), line);
  EXPECT_LT(line, doc.scroll_top() + doc.viewport_height());
  EXPECT_LE(doc.scroll_left(), doc.cursor().column);
  EXPECT_LT(doc.cursor().column, doc.scroll_left() + doc.viewport_width());
}

TEST(EditorDocument, EmptyDocumentIsSingleEmptyLine) {
  EditorDocument doc;
  EXPECT_EQ(doc.line_count(), 1U);
  EXPECT_EQ(doc.line(0), "");
  EXPECT_EQ(doc.text(), "");
  EXPECT_EQ(doc.cursor(), (TextPosition{0, 0}));
}

TEST(EditorDocument, InsertionAtCursor) {
  EditorDocument doc = DocumentFrom({"hlo"});
  doc.insert_text("el");  // inserts at column 0
  EXPECT_EQ(doc.text(), "elhlo");
  EXPECT_EQ(doc.cursor(), (TextPosition{0, 2}));  // after the inserted "el"
}

TEST(EditorDocument, InsertionInMiddleOfLine) {
  EditorDocument doc = DocumentFrom({"hlo"});
  doc.move_right();  // after 'h'
  doc.insert_text("el");
  EXPECT_EQ(doc.text(), "hello");
  EXPECT_EQ(doc.cursor(), (TextPosition{0, 3}));
}

TEST(EditorDocument, NewlineSplitsLine) {
  EditorDocument doc = DocumentFrom({"hello"});
  doc.move_end();
  doc.insert_newline();
  EXPECT_EQ(doc.line_count(), 2U);
  EXPECT_EQ(doc.line(0), "hello");
  EXPECT_EQ(doc.line(1), "");
  EXPECT_EQ(doc.cursor(), (TextPosition{1, 0}));
}

TEST(EditorDocument, NewlineSplitsInMiddle) {
  EditorDocument doc = DocumentFrom({"abcd"});
  doc.move_right();
  doc.move_right();  // col 2
  doc.insert_newline();
  EXPECT_EQ(doc.line(0), "ab");
  EXPECT_EQ(doc.line(1), "cd");
  EXPECT_EQ(doc.cursor(), (TextPosition{1, 0}));
}

TEST(EditorDocument, BackspaceJoinsLinesAtBoundary) {
  EditorDocument doc = DocumentFrom({"line one", "line two"});
  doc.set_cursor({1, 0});
  doc.delete_backward();
  EXPECT_EQ(doc.line_count(), 1U);
  EXPECT_EQ(doc.line(0), "line oneline two");
  EXPECT_EQ(doc.cursor(), (TextPosition{0, 8}));
}

TEST(EditorDocument, DeleteJoinsLinesAtBoundary) {
  EditorDocument doc = DocumentFrom({"line one", "line two"});
  doc.set_cursor({0, 8});
  doc.delete_forward();
  EXPECT_EQ(doc.line_count(), 1U);
  EXPECT_EQ(doc.line(0), "line oneline two");
  EXPECT_EQ(doc.cursor(), (TextPosition{0, 8}));
}

TEST(EditorDocument, BackspaceInsideLine) {
  EditorDocument doc = DocumentFrom({"abcd"});
  doc.move_end();
  doc.delete_backward();
  EXPECT_EQ(doc.line(0), "abc");
  EXPECT_EQ(doc.cursor(), (TextPosition{0, 3}));
}

TEST(EditorDocument, DeleteInsideLine) {
  EditorDocument doc = DocumentFrom({"abcd"});
  doc.move_right();
  doc.delete_forward();  // removes 'b'
  EXPECT_EQ(doc.line(0), "acd");
  EXPECT_EQ(doc.cursor(), (TextPosition{0, 1}));
}

TEST(EditorDocument, EmptyDocumentOperationsAreSafe) {
  EditorDocument doc;
  doc.delete_backward();
  doc.delete_forward();
  doc.insert_newline();
  doc.move_up();
  doc.move_down();
  doc.move_left();
  doc.move_right();
  doc.move_home();
  doc.move_end();
  doc.move_word_left();
  doc.move_word_right();
  EXPECT_EQ(doc.line_count(), 2U);  // one newline was inserted
  EXPECT_EQ(doc.text(), "\n");
  ExpectInvariants(doc);
}

TEST(EditorDocument, AllCursorDirections) {
  EditorDocument doc = DocumentFrom({"abcd", "efgh", "ijkl"});
  doc.set_cursor({1, 2});  // 'g'
  doc.move_up();
  EXPECT_EQ(doc.cursor(), (TextPosition{0, 2}));  // 'c'
  doc.move_down();
  EXPECT_EQ(doc.cursor(), (TextPosition{1, 2}));
  doc.move_down();
  EXPECT_EQ(doc.cursor(), (TextPosition{2, 2}));  // 'k'
  doc.move_right();
  EXPECT_EQ(doc.cursor(), (TextPosition{2, 3}));
  doc.move_left();
  EXPECT_EQ(doc.cursor(), (TextPosition{2, 2}));
}

TEST(EditorDocument, HomeAndEnd) {
  EditorDocument doc = DocumentFrom({"hello world"});
  doc.move_end();
  EXPECT_EQ(doc.cursor(), (TextPosition{0, 11}));
  doc.move_home();
  EXPECT_EQ(doc.cursor(), (TextPosition{0, 0}));
}

TEST(EditorDocument, TopAndBottomLineBoundaries) {
  EditorDocument doc = DocumentFrom({"first", "second"});
  doc.move_up();  // already at top
  EXPECT_EQ(doc.cursor(), (TextPosition{0, 0}));
  doc.move_down();
  doc.move_down();                                // already at bottom
  EXPECT_EQ(doc.cursor(), (TextPosition{1, 6}));  // end of last line
}

TEST(EditorDocument, WordNavigation) {
  EditorDocument doc = DocumentFrom({"hello world foo"});
  doc.move_word_right();
  EXPECT_EQ(doc.cursor().column, 5U);
  doc.move_word_right();
  EXPECT_EQ(doc.cursor().column, 11U);
  doc.move_word_right();
  EXPECT_EQ(doc.cursor().column, 15U);
  doc.move_word_left();
  EXPECT_EQ(doc.cursor().column, 12U);
  doc.move_word_left();
  EXPECT_EQ(doc.cursor().column, 6U);
  doc.move_word_left();
  EXPECT_EQ(doc.cursor().column, 0U);
}

TEST(EditorDocument, PreferredColumnAcrossLinesOfVaryingLength) {
  EditorDocument doc = DocumentFrom({"a", "bbbb", "c"});
  doc.set_cursor({1, 2});
  // Moving up onto a 1-char line clamps the column to 1.
  doc.move_up();
  EXPECT_EQ(doc.cursor(), (TextPosition{0, 1}));
  // Moving down restores the preferred column on the long line.
  doc.move_down();
  EXPECT_EQ(doc.cursor(), (TextPosition{1, 2}));
  // Moving further down onto the 1-char line clamps again.
  doc.move_down();
  EXPECT_EQ(doc.cursor(), (TextPosition{2, 1}));
  // And back up restores the 2-column preference.
  doc.move_up();
  EXPECT_EQ(doc.cursor(), (TextPosition{1, 2}));
}

TEST(EditorDocument, EmptyLinesAreNavigable) {
  EditorDocument doc = DocumentFrom({"a", "", "b"});
  doc.set_cursor({0, 1});
  doc.move_down();
  EXPECT_EQ(doc.cursor(), (TextPosition{1, 0}));  // preferred column clamps to 0
  doc.move_down();
  EXPECT_EQ(doc.cursor(), (TextPosition{2, 1}));  // preferred column 1 restored on "b"
  doc.insert_text("x");
  EXPECT_EQ(doc.line(2), "bx");  // 'x' inserted after 'b' at the cursor
}

TEST(EditorDocument, MultilinePasteSplitsLines) {
  EditorDocument doc;
  doc.insert_text("alpha\nbeta\ngamma");
  EXPECT_EQ(doc.line_count(), 3U);
  EXPECT_EQ(doc.line(0), "alpha");
  EXPECT_EQ(doc.line(1), "beta");
  EXPECT_EQ(doc.line(2), "gamma");
  EXPECT_EQ(doc.cursor(), (TextPosition{2, 5}));
  EXPECT_EQ(doc.text(), "alpha\nbeta\ngamma");
}

TEST(EditorDocument, MultilinePasteIntoExistingLine) {
  EditorDocument doc = DocumentFrom({"xy"});
  doc.move_right();  // col 1
  doc.insert_text("1\n2");
  EXPECT_EQ(doc.line(0), "x1");
  EXPECT_EQ(doc.line(1), "2y");
  EXPECT_EQ(doc.cursor(), (TextPosition{1, 1}));  // after the '2', before 'y'
}

TEST(EditorDocument, Utf8InsertWithoutCorruption) {
  EditorDocument doc;
  doc.insert_text("Привет");
  EXPECT_EQ(doc.line(0), "Привет");
  EXPECT_EQ(doc.line(0).size(), 12U);  // 6 code points * 2 bytes
  EXPECT_EQ(doc.cursor(), (TextPosition{0, 12}));
  // Move left across each code point, then back.
  for (int i = 0; i < 6; ++i) {
    doc.move_left();
  }
  EXPECT_EQ(doc.cursor(), (TextPosition{0, 0}));
  for (int i = 0; i < 6; ++i) {
    doc.move_right();
  }
  EXPECT_EQ(doc.cursor(), (TextPosition{0, 12}));
}

TEST(EditorDocument, Utf8EditInMiddleKeepsBoundaries) {
  EditorDocument doc = DocumentFrom({"aПриветb"});
  // Move right once (past 'a'), then insert 'X'.
  doc.move_right();
  doc.insert_text("X");
  EXPECT_EQ(doc.line(0), "aXПриветb");
  ExpectInvariants(doc);
  // Delete backward should remove the inserted 'X', not mid-codepoint.
  doc.delete_backward();
  EXPECT_EQ(doc.line(0), "aПриветb");
}

TEST(EditorDocument, SetTextSplitsAndResets) {
  EditorDocument doc = DocumentFrom({"old"});
  doc.set_text("line1\nline2");
  EXPECT_EQ(doc.line_count(), 2U);
  EXPECT_EQ(doc.cursor(), (TextPosition{0, 0}));
  EXPECT_EQ(doc.text(), "line1\nline2");
}

TEST(EditorDocument, ClearProducesEmptyDocument) {
  EditorDocument doc = DocumentFrom({"a", "b"});
  doc.clear();
  EXPECT_EQ(doc.line_count(), 1U);
  EXPECT_EQ(doc.line(0), "");
  EXPECT_EQ(doc.cursor(), (TextPosition{0, 0}));
}

TEST(EditorDocument, SetCursorClampsOutOfRange) {
  EditorDocument doc = DocumentFrom({"ab", "cdef"});
  doc.set_cursor({99, 0});
  EXPECT_EQ(doc.cursor().line, 1U);  // clamped to last line
  doc.set_cursor({0, 99});
  EXPECT_EQ(doc.cursor().column, 2U);  // clamped to line length
}

TEST(EditorDocument, SetCursorClampsPreferredColumnToClampedCursor) {
  EditorDocument doc = DocumentFrom({"ab", "cccccccc"});
  doc.set_cursor({0, 99});  // clamped to col 2; preferred must match, not 99
  EXPECT_EQ(doc.preferred_column(), 2U);
  doc.move_down();  // preferred 2 on an 8-char line -> column 2
  EXPECT_EQ(doc.cursor(), (TextPosition{1, 2}));
}

TEST(EditorDocument, ViewportHeightMinClampedToOne) {
  EditorDocument doc = DocumentFrom({"x"});
  doc.set_viewport_size(5, 0);
  EXPECT_EQ(doc.viewport_width(), 5U);
  EXPECT_EQ(doc.viewport_height(), 1U);
}

TEST(EditorDocument, ViewportScrollsVerticallyToCursor) {
  EditorDocument doc;
  std::vector<std::string> lines;
  for (int i = 0; i < 100; ++i) {
    lines.push_back("line " + std::to_string(i));
  }
  doc = DocumentFrom(std::move(lines));
  doc.set_viewport_size(40, 10);
  doc.set_cursor({50, 0});
  EXPECT_EQ(doc.scroll_top(), 41U);
  EXPECT_LT(doc.cursor().line, doc.scroll_top() + doc.viewport_height());
  // Moving up keeps the cursor visible.
  doc.move_up();
  EXPECT_GE(doc.cursor().line, doc.scroll_top());
}

TEST(EditorDocument, ViewportScrollsHorizontallyToCursor) {
  EditorDocument doc = DocumentFrom({std::string(200, 'x')});
  doc.set_viewport_size(20, 5);
  doc.set_cursor({0, 150});
  EXPECT_LE(doc.scroll_left(), 150U);
  EXPECT_LT(150U, doc.scroll_left() + doc.viewport_width());
}

TEST(EditorDocument, ResizeKeepsCursorVisible) {
  EditorDocument doc;
  std::vector<std::string> lines;
  for (int i = 0; i < 50; ++i) {
    lines.push_back("row");
  }
  doc = DocumentFrom(std::move(lines));
  doc.set_viewport_size(40, 5);
  doc.set_cursor({49, 0});
  EXPECT_EQ(doc.scroll_top(), 45U);
  // Shrink the viewport: the cursor must be pulled back into view.
  doc.set_viewport_size(40, 2);
  EXPECT_GE(doc.cursor().line, doc.scroll_top());
  EXPECT_LT(doc.cursor().line, doc.scroll_top() + doc.viewport_height());
}

TEST(EditorDocument, LargeBufferRoundTrip) {
  std::vector<std::string> lines;
  lines.reserve(100000);
  for (std::size_t i = 0; i < 100000; ++i) {
    lines.push_back("row " + std::to_string(i));
  }
  EditorDocument doc = DocumentFrom(lines);
  EXPECT_EQ(doc.line_count(), 100000U);
  const std::string whole = doc.text();
  std::size_t expected_size = 99999;  // separators
  for (const std::string& l : lines) {
    expected_size += l.size();
  }
  EXPECT_EQ(whole.size(), expected_size);
  // Insert a large single-line block efficiently at the front.
  doc.set_cursor({50000, 0});
  doc.insert_text(std::string(200000, 'Z'));
  EXPECT_EQ(doc.line(50000).size(), 200000UL + std::string("row 50000").size());
  ExpectInvariants(doc);
}

TEST(EditorDocument, LargeMultilineInsertIsNotQuadratic) {
  // Inserting many lines into a document must be a single O(n) splice, not an
  // O(n^2) rebuild. Verified functionally: intermediate lines are preserved and
  // the tail of the destination line is carried to the final inserted line.
  EditorDocument doc = DocumentFrom({"head", "tail"});
  doc.set_cursor({1, 0});
  std::string paste;
  const int kLines = 20000;
  for (int i = 0; i < kLines; ++i) {
    paste += "l" + std::to_string(i) + "\n";
  }
  paste += "last";
  doc.insert_text(paste);
  EXPECT_EQ(doc.line_count(), 2U + static_cast<std::size_t>(kLines));
  // The first inserted line is at index 1 (after "head"), the tail of the old
  // line 1 ("tail") is appended to the final inserted line.
  EXPECT_EQ(doc.line(1), "l0");
  EXPECT_EQ(doc.line(1 + kLines), "lasttail");
  EXPECT_EQ(doc.line(0), "head");
  ExpectInvariants(doc);
}

TEST(EditorDocument, TextMatchesReplay) {
  EditorDocument doc;
  doc.insert_text("one\ntwo\nthree");
  EXPECT_EQ(doc.line(0), "one");
  EXPECT_EQ(doc.line(1), "two");
  EXPECT_EQ(doc.line(2), "three");
}

namespace {
enum class Op {
  kInsert,
  kNewline,
  kBackspace,
  kDelete,
  kLeft,
  kRight,
  kUp,
  kDown,
  kHome,
  kEnd,
  kWordLeft,
  kWordRight,
  kSetCursor,
  kResize
};
}

TEST(EditorDocument, RandomizedOperationsPreserveInvariants) {
  std::mt19937 rng(12345);
  auto rand = [&](std::size_t lo, std::size_t hi) {
    return lo + static_cast<std::size_t>(rng()) % (hi - lo + 1);
  };

  EditorDocument doc = DocumentFrom({"start"});
  doc.set_viewport_size(15, 6);
  const Op ops[] = {Op::kInsert,   Op::kNewline,   Op::kBackspace, Op::kDelete, Op::kLeft,
                    Op::kRight,    Op::kUp,        Op::kDown,      Op::kHome,   Op::kEnd,
                    Op::kWordLeft, Op::kWordRight, Op::kSetCursor, Op::kResize};

  for (int step = 0; step < 2000; ++step) {
    switch (ops[rand(0, sizeof(ops) / sizeof(ops[0]) - 1)]) {
      case Op::kInsert:
        doc.insert_text(std::string(rand(1, 6), static_cast<char>('a' + rand(0, 3))));
        break;
      case Op::kNewline:
        doc.insert_newline();
        break;
      case Op::kBackspace:
        doc.delete_backward();
        break;
      case Op::kDelete:
        doc.delete_forward();
        break;
      case Op::kLeft:
        doc.move_left();
        break;
      case Op::kRight:
        doc.move_right();
        break;
      case Op::kUp:
        doc.move_up();
        break;
      case Op::kDown:
        doc.move_down();
        break;
      case Op::kHome:
        doc.move_home();
        break;
      case Op::kEnd:
        doc.move_end();
        break;
      case Op::kWordLeft:
        doc.move_word_left();
        break;
      case Op::kWordRight:
        doc.move_word_right();
        break;
      case Op::kSetCursor:
        doc.set_cursor({rand(0, doc.line_count() + 2), rand(0, 40)});
        break;
      case Op::kResize:
        doc.set_viewport_size(rand(1, 30), rand(1, 10));
        break;
    }
    ExpectInvariants(doc);
  }
}

TEST(EditorDocument, RandomizedBracketedPastePreservesDocument) {
  EditorDocument doc;
  doc.set_viewport_size(20, 5);
  // Build an expected buffer from the same paste to compare text() output.
  std::string expected;
  for (int i = 0; i < 50; ++i) {
    const std::string chunk = "p" + std::to_string(i) + "\n";
    doc.insert_text(chunk);
    expected += chunk;
    ExpectInvariants(doc);
  }
  // The document must reconstruct exactly what was pasted (append only here).
  EXPECT_EQ(doc.text(), expected);
}

}  // namespace
}  // namespace terminal_ui_kit
