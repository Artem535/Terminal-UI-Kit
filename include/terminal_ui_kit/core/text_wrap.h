#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace terminal_ui_kit {

// Wraps `text` to at most `width` Unicode codepoints per line, breaking on
// ASCII space/tab runs when possible (the separating whitespace is dropped,
// matching typical word-wrap behavior). A single "word" longer than `width`
// is hard-broken at a codepoint boundary -- multi-byte UTF-8 sequences are
// never split. Empty input yields a single empty line.
//
// This counts one display column per codepoint; wide characters (CJK,
// emoji) and combining marks are not accounted for -- that needs a
// maintained Unicode width table (utf8proc, see PRD section 16.2 and open
// question 62.2) and is deferred to the Text Layout Engine (section 16).
std::vector<std::string> wrap_plain_text(std::string_view text, int width);

}  // namespace terminal_ui_kit
