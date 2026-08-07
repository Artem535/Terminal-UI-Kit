#include "terminal_ui_kit/search/search_engine.h"

#include <algorithm>
#include <cstddef>
#include <regex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace terminal_ui_kit {

namespace {

// Folds one byte for ASCII case-insensitive comparison. Bytes >= 0x80 (i.e.
// any UTF-8 continuation or multi-byte lead) are left untouched, so non-ASCII
// code points always compare case-sensitively.
char fold_ascii(unsigned char byte) {
  if (byte >= 'A' && byte <= 'Z') {
    return static_cast<char>(byte + ('a' - 'A'));
  }
  return static_cast<char>(byte);
}

std::string fold_query_ascii(std::string_view query) {
  std::string folded;
  folded.reserve(query.size());
  for (char byte : query) {
    folded.push_back(fold_ascii(static_cast<unsigned char>(byte)));
  }
  return folded;
}

// Byte-wise, ASCII-folding substring search. `needle` must already be folded.
// Returns the byte offset of the first occurrence at/after `from`, or npos.
std::size_t find_folded(std::string_view haystack, std::string_view folded_needle,
                        std::size_t from) {
  if (folded_needle.empty()) {
    return from;
  }
  if (folded_needle.size() > haystack.size()) {
    return std::string_view::npos;
  }
  const std::size_t last_possible_plus_one = haystack.size() - folded_needle.size() + 1;
  for (std::size_t i = from; i < last_possible_plus_one; ++i) {
    bool matched = true;
    for (std::size_t j = 0; j < folded_needle.size(); ++j) {
      if (fold_ascii(static_cast<unsigned char>(haystack[i + j])) != folded_needle[j]) {
        matched = false;
        break;
      }
    }
    if (matched) {
      return i;
    }
  }
  return std::string_view::npos;
}

SearchStatus literal_search(std::span<const std::string_view> lines, std::string_view query,
                            const SearchOptions& options, std::vector<TextMatch>& out) {
  out.clear();
  const std::string folded_query =
      options.case_sensitive ? std::string(query) : fold_query_ascii(query);

  for (std::size_t line_index = 0; line_index < lines.size(); ++line_index) {
    const std::string_view line = lines[line_index];
    std::size_t pos = 0;
    while (pos <= line.size()) {
      std::size_t found;
      if (options.case_sensitive) {
        found = line.find(query, pos);
      } else {
        found = find_folded(line, folded_query, pos);
      }
      if (found == std::string_view::npos) {
        break;
      }
      out.push_back(TextMatch{line_index, found, found + query.size()});
      pos = found + query.size();
    }
  }
  return out.empty() ? SearchStatus::kNoResults : SearchStatus::kMatches;
}

SearchStatus regex_search(std::span<const std::string_view> lines, std::string_view query,
                          const SearchOptions& options, std::vector<TextMatch>& out) {
  out.clear();
  std::regex::flag_type flags = std::regex_constants::ECMAScript;
  if (!options.case_sensitive) {
    flags |= std::regex_constants::icase;
  }
  std::regex pattern;
  try {
    pattern.assign(std::string(query), flags);
  } catch (const std::regex_error&) {
    return SearchStatus::kInvalidRegex;
  }

  for (std::size_t line_index = 0; line_index < lines.size(); ++line_index) {
    const std::string_view line = lines[line_index];
    // std::regex iterators require a null-terminated buffer.
    const std::string line_buffer(line);
    const std::sregex_iterator end;
    for (std::sregex_iterator it(line_buffer.begin(), line_buffer.end(), pattern); it != end;
         ++it) {
      const std::smatch& match = *it;
      const std::size_t start = static_cast<std::size_t>(match.position());
      const std::size_t length = static_cast<std::size_t>(match.length());
      if (length == 0) {
        continue;  // skip zero-length matches
      }
      out.push_back(TextMatch{line_index, start, start + length});
    }
  }
  return out.empty() ? SearchStatus::kNoResults : SearchStatus::kMatches;
}

}  // namespace

SearchStatus SearchEngine::search(std::span<const std::string_view> lines, std::string_view query,
                                  const SearchOptions& options,
                                  std::vector<TextMatch>& out_matches) {
  if (query.empty()) {
    out_matches.clear();
    return SearchStatus::kEmptyQuery;
  }
  if (options.use_regex) {
    return regex_search(lines, query, options, out_matches);
  }
  return literal_search(lines, query, options, out_matches);
}

void MatchNavigator::set_matches(std::vector<TextMatch> matches) {
  const std::optional<TextMatch> previous = current();
  const std::optional<std::size_t> previous_index = current_;

  matches_ = std::move(matches);
  if (matches_.empty()) {
    current_.reset();
    return;
  }

  if (previous && previous_index) {
    const auto it = std::find(matches_.begin(), matches_.end(), *previous);
    if (it != matches_.end()) {
      current_ = static_cast<std::size_t>(it - matches_.begin());
      return;
    }
    current_ = std::min(*previous_index, matches_.size() - 1);
    return;
  }
  current_ = 0;
}

void MatchNavigator::clear() {
  matches_.clear();
  current_.reset();
}

void MatchNavigator::set_current(std::size_t index) {
  if (matches_.empty()) {
    return;
  }
  current_ = std::min(index, matches_.size() - 1);
}

std::optional<TextMatch> MatchNavigator::current() const {
  if (!current_ || *current_ >= matches_.size()) {
    return std::nullopt;
  }
  return matches_[*current_];
}

void MatchNavigator::next() {
  if (matches_.empty()) {
    return;
  }
  if (!current_) {
    current_ = 0;
    return;
  }
  current_ = (*current_ + 1) % matches_.size();
}

void MatchNavigator::previous() {
  if (matches_.empty()) {
    return;
  }
  if (!current_) {
    current_ = 0;
    return;
  }
  current_ = (*current_ == 0) ? matches_.size() - 1 : *current_ - 1;
}

void MatchNavigator::jump_to_first() {
  if (!matches_.empty()) {
    current_ = 0;
  }
}

}  // namespace terminal_ui_kit
