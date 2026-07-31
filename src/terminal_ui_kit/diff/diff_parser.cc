#include "terminal_ui_kit/diff/diff_parser.h"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace terminal_ui_kit {
namespace diff {
namespace {

/// Strip leading 'a/' or 'b/' from a git-tracked path such as
/// "a/foo.txt" or "b/foo.txt". Returns the stripped path as-is
/// when it does not start with those prefixes (e.g. "/dev/null").
std::string strip_prefix(std::string_view path) {
  if (path.size() >= 2 &&
      ((path[0] == 'a' && path[1] == '/') || (path[0] == 'b' && path[1] == '/'))) {
    return std::string(path.substr(2));
  }
  return std::string(path);
}

/// Parse a hunk-index fragment like "-1,5", "1,5", or ",5".
/// Writes (number, length) and returns the remainder (normally empty).
std::string_view parse_hunk_index(std::string_view idx, std::uint32_t& number,
                                  std::uint32_t& length) {
  auto p = idx.begin();
  const auto end = idx.end();

  // Skip optional '+' or '-' prefix from "git diff".
  if (p != end && (*p == '-' || *p == '+')) {
    ++p;
  }

  // --- start ---
  const auto start_begin = p;
  while (p != end && *p >= '0' && *p <= '9') ++p;
  if (start_begin == p) {
    number = 0;
  } else {
    std::from_chars(start_begin, p, number);
  }

  // Must have comma followed by length digits.
  if (p == end || *p != ',') return {};
  ++p;
  const auto len_begin = p;
  while (p != end && *p >= '0' && *p <= '9') ++p;
  if (len_begin == p) return {};
  std::uint32_t tmp = 0;
  std::from_chars(len_begin, p, tmp);
  length = tmp;
  return {p, static_cast<std::size_t>(end - p)};
}

}  // namespace

Diff parse_unified_diff(std::string_view input) {
  Diff result;
  DiffFile current_file;

  auto flush_file = [&]() {
    if (!current_file.hunks.empty() || current_file.is_binary || !current_file.header.empty()) {
      result.files.push_back(std::move(current_file));
      current_file = DiffFile{};
    }
  };

  // ---------- split into lines (handles \n, \r\n) -------------------------
  std::vector<std::string_view> raw_lines;
  {
    std::size_t start = 0;
    for (std::size_t i = 0; i <= input.size(); ++i) {
      if (i == input.size() || input[i] == '\n' || input[i] == '\r') {
        raw_lines.emplace_back(input.data() + start, i - start);
        // Strip trailing \r for \r\n.
        if (!raw_lines.back().empty() && raw_lines.back().back() == '\r') {
          raw_lines.back() = {raw_lines.back().data(), raw_lines.back().size() - 1};
        }
        start = i + 1;
        continue;
      }
    }
  }

  // ---------- state machine -----------------------------------------------
  for (const auto& raw : raw_lines) {
    if (raw.empty()) {
      continue;
    }

    // --- diff --git header ------------------------------------------------
    if (raw.size() >= 11 && raw.substr(0, 11) == "diff --git ") {
      flush_file();
      current_file.header = std::string(raw);

      auto sep_a = raw.find(" a/");
      if (sep_a != std::string_view::npos) {
        auto sep_b = raw.find(" b/", sep_a + 3);
        if (sep_b != std::string_view::npos) {
          current_file.old_path = strip_prefix(raw.substr(sep_a + 3, sep_b - sep_a - 3));
          current_file.new_path = strip_prefix(raw.substr(sep_b + 3));
        }
      }
      continue;
    }

    // --- --- a/path / --- /dev/null ---------------------------------------
    if (raw.size() >= 4 && raw.substr(0, 4) == "--- ") {
      current_file.old_path = strip_prefix(raw.substr(4));
      continue;
    }

    // --- +++ b/path / +++ /dev/null ---------------------------------------
    if (raw.size() >= 4 && raw.substr(0, 4) == "+++ ") {
      current_file.new_path = strip_prefix(raw.substr(4));
      continue;
    }

    // --- Binary file notice -----------------------------------------------
    if (raw.size() >= 13 && raw.substr(0, 13) == "Binary files ") {
      current_file.is_binary = true;
      current_file.hunks.clear();
      continue;
    }

    // --- Hunk header "@@ -old @@" -----------------------------------------
    if (raw.size() >= 6 && raw[0] == '@' && raw[1] == '@') {
      std::size_t sp1 = raw.find(' ', 2);
      if (sp1 == std::string_view::npos) continue;
      std::size_t sp2 = raw.find(' ', sp1 + 1);
      if (sp2 == std::string_view::npos) continue;

      const std::string_view old_raw = raw.substr(sp1 + 1, sp2 - sp1 - 1);
      // Everything from after sp2 up to (and including) trailing "@@".
      const auto trailing_at = raw.find("@@", sp2 + 1);
      const std::string_view new_raw = (trailing_at != std::string_view::npos)
                                           ? raw.substr(sp2 + 1, trailing_at - sp2 - 1)
                                           : raw.substr(sp2 + 1);

      std::uint32_t old_start = 0, old_len = 0;
      std::uint32_t new_start = 0, new_len = 0;
      parse_hunk_index(old_raw, old_start, old_len);
      parse_hunk_index(new_raw, new_start, new_len);

      Hunk hunk{};
      hunk.old_start = old_start;
      hunk.old_length = old_len;
      hunk.new_start = new_start;
      hunk.new_length = new_len;
      hunk.header = std::string(raw);
      current_file.hunks.push_back(std::move(hunk));
      continue;
    }

    // --- Hunk lines: ' ' context, '+' addition, '-' deletion ---------------
    if (!current_file.hunks.empty()) {
      auto& line = current_file.hunks.back().lines.emplace_back();
      if (raw.size() >= 1) {
        line.content = std::string(raw.substr(1));
      } else {
        line.content = {};
        continue;  // nothing to consume
      }
      if (raw[0] == ' ') {
        line.type = LineType::kContext;
      } else if (raw[0] == '+') {
        line.type = LineType::kAddition;
      } else if (raw[0] == '-') {
        line.type = LineType::kDeletion;
      }
      // Any other character at the start of a hunk line is silently
      // dropped (e.g. stray "@@" footer text).
      continue;
    }

    // --- Everything else: skip ---------------------------------------------
    // "index …", "new/deleted file mode …", similarity/size info,
    // "GIT binary patch", stray whitespace lines… all tolerated.
  }

  flush_file();
  return result;
}

}  // namespace diff
}  // namespace terminal_ui_kit
