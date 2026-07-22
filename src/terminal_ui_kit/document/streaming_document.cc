#include "terminal_ui_kit/document/streaming_document.h"

#include <stdexcept>

namespace terminal_ui_kit {
namespace {

constexpr char kReplacement[] = "\xEF\xBF\xBD";

bool is_continuation(unsigned char byte) { return (byte & 0xC0U) == 0x80U; }

int sequence_length(unsigned char byte) {
  if (byte < 0x80U) return 1;
  if (byte >= 0xC2U && byte <= 0xDFU) return 2;
  if (byte >= 0xE0U && byte <= 0xEFU) return 3;
  if (byte >= 0xF0U && byte <= 0xF4U) return 4;
  return 0;
}

}  // namespace

void StreamingDocument::append(std::string_view chunk) {
  append_bytes(chunk);
  ++revision_;
}

void StreamingDocument::append_bytes(std::string_view bytes) {
  pending_bytes_.append(bytes.data(), bytes.size());
  decode_pending(false);
}

void StreamingDocument::decode_pending(bool flush_incomplete) {
  std::size_t offset = 0;
  while (offset < pending_bytes_.size()) {
    const auto lead = static_cast<unsigned char>(pending_bytes_[offset]);
    if (lead == '\n') {
      ++offset;
      finish_line();
      continue;
    }

    const int length = sequence_length(lead);
    if (length == 0) {
      current_line_.append(kReplacement);
      ++offset;
      continue;
    }
    if (offset + static_cast<std::size_t>(length) > pending_bytes_.size()) {
      if (!flush_incomplete) break;
      current_line_.append(kReplacement);
      ++offset;
      continue;
    }

    bool valid = true;
    for (int index = 1; index < length; ++index) {
      if (!is_continuation(static_cast<unsigned char>(
              pending_bytes_[offset + static_cast<std::size_t>(index)]))) {
        valid = false;
        break;
      }
    }
    // Reject overlong encodings, surrogates, and codepoints above U+10FFFF.
    if (valid && length == 3) {
      const auto second = static_cast<unsigned char>(pending_bytes_[offset + 1]);
      valid = !(lead == 0xE0U && second < 0xA0U) && !(lead == 0xEDU && second >= 0xA0U);
    } else if (valid && length == 4) {
      const auto second = static_cast<unsigned char>(pending_bytes_[offset + 1]);
      valid = !(lead == 0xF0U && second < 0x90U) && !(lead == 0xF4U && second > 0x8FU);
    }
    if (!valid) {
      current_line_.append(kReplacement);
      ++offset;
      continue;
    }

    current_line_.append(pending_bytes_, offset, static_cast<std::size_t>(length));
    offset += static_cast<std::size_t>(length);
  }
  pending_bytes_.erase(0, offset);
  if (flush_incomplete && !pending_bytes_.empty()) {
    current_line_.append(kReplacement);
    pending_bytes_.clear();
  }
}

void StreamingDocument::finish_line() {
  if (!current_line_.empty() && current_line_.back() == '\r') {
    current_line_.pop_back();
  }
  lines_.push_back(std::move(current_line_));
  current_line_.clear();
}

void StreamingDocument::replace_tail(std::string_view tail) {
  current_line_.clear();
  pending_bytes_.clear();
  append_bytes(tail);
  ++revision_;
}

void StreamingDocument::finish() {
  decode_pending(true);
  finish_line();
  ++revision_;
}

void StreamingDocument::clear() {
  lines_.clear();
  current_line_.clear();
  pending_bytes_.clear();
  ++revision_;
}

std::string_view StreamingDocument::line_at(std::size_t index) const {
  if (index >= lines_.size()) {
    throw std::out_of_range("StreamingDocument::line_at");
  }
  return lines_[index];
}

}  // namespace terminal_ui_kit
