#include "terminal_ui_kit/diff/diff_parser.h"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace terminal_ui_kit {
namespace {

// Strip optional "a/" or "b/" prefix from a git diff path.
std::string_view strip_git_prefix(std::string_view path) {
  if (path.starts_with("a/")) path.remove_prefix(2);
  if (path.starts_with("b/")) path.remove_prefix(2);
  return path;
}

// Parse an old/new range spec like "1,3" or "0" into start and count.
// The caller must strip leading '+' or '-' before invoking.
static bool parse_range(std::string_view spec, std::uint32_t& start, std::uint32_t& count) {
  const auto comma = spec.find(',');
  if (comma == std::string_view::npos) {
    // Single number: e.g. "1"
    const auto result = std::from_chars(spec.data(), spec.data() + spec.size(), start);
    if (result.ec != std::errc{}) return false;
    count = 0;
    return true;
  }
  // "1,3"
  const auto result_start = std::from_chars(spec.data(), spec.data() + comma, start);
  if (result_start.ec != std::errc{}) return false;
  const auto result_count =
      std::from_chars(spec.data() + comma + 1, spec.data() + spec.size(), count);
  if (result_count.ec != std::errc{}) return false;
  return true;
}

// Parse @@ ... @@ into a DiffHunk.
bool parse_hunk_header(std::string_view header, DiffHunk& hunk) {
  if (header.size() < 10 || header[0] != '@' || header[1] != '@') return false;

  // Find the closing "@@" (the second occurrence after position 1).
  const auto second_at_at = header.find("@@", 2);
  if (second_at_at == std::string_view::npos) return false;

  // Everything after the closing "@@" is the optional context string.
  hunk.context = std::string(header.substr(second_at_at + 2));

  // Body is everything between the opening "@@" and closing "@@".
  std::string_view body = header.substr(2, second_at_at - 2);

  // Trim leading whitespace.
  auto first = body.find_first_not_of(' ');
  if (first == std::string_view::npos) return false;
  body.remove_prefix(first);

  // Find the space separating the old and new range specs.
  const auto space = body.find(' ');
  if (space == std::string_view::npos) return false;

  std::string_view old_spec = body.substr(0, space);
  std::string_view new_spec = body.substr(space + 1);

  // Strip leading '-' or '+' from unified-diff range specs.
  if (!old_spec.empty() && (old_spec[0] == '-' || old_spec[0] == '+')) old_spec.remove_prefix(1);
  if (!new_spec.empty() && (new_spec[0] == '-' || new_spec[0] == '+')) new_spec.remove_prefix(1);

  if (!parse_range(old_spec, hunk.old_start, hunk.old_count)) return false;
  if (!parse_range(new_spec, hunk.new_start, hunk.new_count)) return false;
  return true;
}

// Trim leading and trailing whitespace from a string_view.
std::string_view trim(std::string_view s) {
  auto start = s.find_first_not_of(" \t");
  if (start == std::string_view::npos) return {};
  auto end = s.find_last_not_of(" \t");
  return s.substr(start, end - start + 1);
}

}  // namespace

std::size_t DiffDocument::total_line_count() const {
  std::size_t count = 0;
  for (const auto& file : files)
    for (const auto& hunk : file.hunks) count += hunk.lines.size();
  return count;
}

DiffDocument::Counts DiffDocument::line_counts() const {
  Counts c;
  for (const auto& file : files)
    for (const auto& hunk : file.hunks)
      for (const auto& line : hunk.lines) switch (line.kind) {
          case DiffLineKind::kAddition:
            ++c.additions;
            break;
          case DiffLineKind::kDeletion:
            ++c.deletions;
            break;
          case DiffLineKind::kContext:
            ++c.context;
            break;
          default:
            break;
        }
  return c;
}

DiffDocument parse_unified_diff(std::string_view input) {
  DiffDocument doc;
  if (input.empty()) return doc;

  // Split into lines.
  std::vector<std::string_view> raw_lines;
  {
    std::string_view r = input;
    while (!r.empty()) {
      const auto nl = r.find('\n');
      if (nl == std::string_view::npos) {
        raw_lines.push_back(r);
        break;
      }
      raw_lines.push_back(r.substr(0, nl));
      r.remove_prefix(nl + 1);
    }
  }

  // Strip trailing \r\n.
  for (auto& l : raw_lines)
    while (!l.empty() && (l.back() == '\n' || l.back() == '\r')) l.remove_suffix(1);

  DiffFile* cur_file = nullptr;
  DiffHunk* cur_hunk = nullptr;

  // Helper to lazily create a file when rename/copy metadata appears
  // before "--- ---".
  auto get_or_create_file = [&]() -> DiffFile* {
    if (!cur_file) cur_file = &doc.files.emplace_back();
    return cur_file;
  };

  for (std::size_t i = 0; i < raw_lines.size(); ++i) {
    const auto& line = raw_lines[i];

    // Skip "diff --git ..." meta lines.
    if (line.starts_with("diff --git")) {
      cur_file = nullptr;
      cur_hunk = nullptr;
      continue;
    }

    // --- "rename from" / "rename to" / "copy from" / "copy to" ---
    if (line.starts_with("rename from ")) {
      auto* f = get_or_create_file();
      f->old_path = std::string(line.substr(12));
      f->is_rename = true;
      continue;
    }
    if (line.starts_with("rename to ")) {
      auto* f = get_or_create_file();
      f->new_path = std::string(line.substr(10));
      continue;
    }
    if (line.starts_with("copy from ")) {
      auto* f = get_or_create_file();
      f->old_path = std::string(line.substr(10));
      f->is_copy = true;
      continue;
    }
    if (line.starts_with("copy to ")) {
      auto* f = get_or_create_file();
      f->new_path = std::string(line.substr(8));
      continue;
    }

    // --- "--- a/path" or "--- /dev/null" ---
    if (line.size() >= 4 && line[0] == '-' && line[1] == '-' && line[2] == '-' && line[3] == ' ') {
      // Peek one line ahead for "+++".
      std::string_view new_path_str;
      if (i + 1 < raw_lines.size()) {
        const auto& nxt = raw_lines[i + 1];
        if (nxt.size() >= 4 && nxt[0] == '+' && nxt[1] == '+' && nxt[2] == '+' && nxt[3] == ' ') {
          const auto peek_path = strip_git_prefix(nxt.substr(4));
          if (peek_path != "/dev/null") new_path_str = peek_path;
        }
      }

      // If cur_file already exists (e.g. from rename from/to), reuse it.
      if (!cur_file) cur_file = &doc.files.emplace_back();
      cur_file->old_path = std::string(strip_git_prefix(line.substr(4)));
      if (!new_path_str.empty()) {
        cur_file->new_path = std::string(new_path_str);
      } else {
        cur_file->new_path = cur_file->old_path;
      }
      cur_hunk = nullptr;
      continue;
    }

    // --- "+++ b/path" ---
    if (line.size() >= 4 && line[0] == '+' && line[1] == '+' && line[2] == '+' && line[3] == ' ') {
      // "+++ /dev/null" means file deletion (not binary).
      if (cur_file && line.substr(4) == "/dev/null") {
        // Mark as deletion candidate — no binary flag needed.
      }
      continue;
    }

    // --- "@@ ..." hunk header ---
    if (line.size() >= 10 && line[0] == '@' && line[1] == '@') {
      if (!cur_file) cur_file = &doc.files.emplace_back();
      cur_hunk = &cur_file->hunks.emplace_back();
      if (!parse_hunk_header(line, *cur_hunk)) cur_hunk = nullptr;
      continue;
    }

    // --- "Binary files X and Y differ" ---
    if (line.starts_with("Binary files ")) {
      if (!cur_file) cur_file = &doc.files.emplace_back();
      cur_file->new_is_binary = true;
      if (cur_file->old_path.empty()) {
        const auto a_pos = line.find(" and ");
        if (a_pos != std::string_view::npos) {
          // "Binary files <a> and <b> differ"
          const std::string_view before_and = line.substr(12, a_pos - 12);
          const std::string_view after_and = line.substr(a_pos + 5);
          const auto d_pos = after_and.find(" differ");
          cur_file->old_path = std::string(strip_git_prefix(trim(before_and)));
          cur_file->new_path = d_pos != std::string_view::npos
                                   ? std::string(strip_git_prefix(trim(after_and.substr(0, d_pos))))
                                   : std::string(strip_git_prefix(trim(after_and)));
        }
      }
      continue;
    }

    // --- index, old mode, new mode ---
    if (line.starts_with("index ")) continue;
    if (line.starts_with("old mode ")) {
      if (cur_file) cur_file->deleted_file_mode = std::string(line.substr(9));
    } else if (line.starts_with("new mode ")) {
      if (cur_file) cur_file->new_file_mode = std::string(line.substr(9));
    } else if (line.starts_with("mode ")) {
      if (cur_file) cur_file->change_description = std::string(line.substr(5));
    }

    // --- Hunk content lines (+, -, space) ---
    if (cur_hunk && cur_file && !line.empty()) {
      const char p = line[0];
      if (p == '+' || p == '-' || p == ' ') {
        DiffLine dl;
        dl.kind = p == '+'   ? DiffLineKind::kAddition
                  : p == '-' ? DiffLineKind::kDeletion
                             : DiffLineKind::kContext;
        // Strip the leading prefix character uniformly for all kinds.
        dl.text = line.substr(1);
        cur_hunk->lines.push_back(std::move(dl));
        continue;
      }
    }
  }

  return doc;
}

}  // namespace terminal_ui_kit
