#include "terminal_ui_kit/diff/diff_model.h"

#include <cstddef>
#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace terminal_ui_kit {

namespace {

// Strip trailing \r and \n.
static std::string_view strip_trailing_crnl(std::string_view sv) {
  while (!sv.empty() && (sv.back() == '\n' || sv.back() == '\r')) {
    sv.remove_suffix(1);
  }
  return sv;
}

// Split input into non-empty lines.
static std::vector<std::string_view> split_lines(std::string_view input) {
  std::vector<std::string_view> result;
  std::string_view remaining = input;
  while (!remaining.empty()) {
    auto nl = remaining.find('\n');
    if (nl == std::string_view::npos) {
      auto line = strip_trailing_crnl(remaining);
      if (!line.empty()) result.push_back(line);
      break;
    }
    auto line = strip_trailing_crnl(remaining.substr(0, nl));
    remaining.remove_prefix(nl + 1);
    if (!line.empty()) result.push_back(line);
  }
  return result;
}

// Parse a range string like "5" or "5,3" into start and count.
static void parse_range(std::string_view range_str, int& start, int& count) {
  std::string s(range_str);
  auto comma = s.find(',');
  char* end = nullptr;
  start = static_cast<int>(strtol(s.c_str(), &end, 10));
  count = 1;  // default
  if (comma != std::string_view::npos && comma + 1 < s.size()) {
    count = static_cast<int>(strtol(s.substr(comma + 1).c_str(), &end, 10));
  }
}

bool try_consume_git_header(std::string_view line, DiffFile& file) {
  constexpr std::string_view kPrefix = "diff --git ";
  if (!line.starts_with(kPrefix)) return false;
  line.remove_prefix(kPrefix.size());

  auto space = line.find(' ');
  if (space == std::string_view::npos) return false;

  std::string_view a_part = line.substr(0, space);
  std::string_view b_part = line.substr(space + 1);

  if (a_part.starts_with("a/")) a_part.remove_prefix(2);
  if (b_part.starts_with("b/")) b_part.remove_prefix(2);

  file.old_path = std::string(a_part);
  file.new_path = std::string(b_part);
  return true;
}

bool try_consume_old_file(std::string_view line, DiffFile& file) {
  constexpr std::string_view kPrefix = "--- ";
  if (!line.starts_with(kPrefix)) return false;
  line.remove_prefix(kPrefix.size());

  // Timestamp separator: two+ spaces OR a TAB
  auto ts = line.find("  ");
  if (ts == std::string_view::npos) ts = line.find('\t');
  if (ts != std::string_view::npos) line = line.substr(0, ts);

  if (line == "/dev/null") return false;

  auto slash = line.find('/');
  if (slash != std::string_view::npos) line.remove_prefix(slash + 1);
  file.old_path = std::string(line);
  return true;
}

bool try_consume_new_file(std::string_view line, DiffFile& file) {
  constexpr std::string_view kPrefix = "+++ ";
  if (!line.starts_with(kPrefix)) return false;
  line.remove_prefix(kPrefix.size());

  // Timestamp separator: two+ spaces OR a TAB
  auto ts = line.find("  ");
  if (ts == std::string_view::npos) ts = line.find('\t');
  if (ts != std::string_view::npos) line = line.substr(0, ts);

  if (line == "/dev/null") {
    file.type = DiffFileType::kDeletedFile;
    return true;
  }

  auto slash = line.find('/');
  if (slash != std::string_view::npos) line.remove_prefix(slash + 1);
  file.new_path = std::string(line);
  return true;
}

DiffChangeType classify_line_char(char c) {
  switch (c) {
    case '+': return DiffChangeType::kAddition;
    case '-': return DiffChangeType::kDeletion;
    case ' ': return DiffChangeType::kContext;
    default:  return DiffChangeType::kUnknown;
  }
}

// Try to parse "@@ -old_range +new_range @@ ..."
std::optional<DiffHunk> try_parse_hunk_header(std::string_view line) {
  constexpr std::string_view kPrefix = "@@ -";
  if (!line.starts_with(kPrefix)) return std::nullopt;
  line.remove_prefix(kPrefix.size());

  auto plus_pos = line.find('+');
  if (plus_pos == std::string_view::npos) return std::nullopt;

  DiffHunk hunk;

  // Old range: everything from the beginning to '+'.
  parse_range(line.substr(0, plus_pos), hunk.old_start, hunk.old_count);

  // New range: everything from '+' to the first space or end.
  auto new_end = line.find(' ', plus_pos + 1);
  if (new_end == std::string_view::npos) new_end = line.size();
  parse_range(line.substr(plus_pos + 1, new_end - (plus_pos + 1)),
              hunk.new_start, hunk.new_count);

  return hunk;
}

}  // namespace

// --- public API ---

DiffModel DiffModel::parse(std::string_view input) {
  DiffModel model;
  DiffFile* current_file = nullptr;
  DiffHunk* current_hunk = nullptr;

  auto lines = split_lines(input);

  for (auto line : lines) {
    // --- File-level headers (outside hunks) ---

    // diff --git
    if (line.starts_with("diff --git ")) {
      current_file = &model.files.emplace_back();
      try_consume_git_header(line, *current_file);
      current_hunk = nullptr;
      continue;
    }

    // index — metadata, skip
    if (line.starts_with("index ")) {
      continue;
    }

    // new file mode / deleted file mode
    if (line.starts_with("new file mode ")) {
      if (current_file) {
        current_file->type = DiffFileType::kNewFile;
        auto rest = line.substr(14);
        auto sp = rest.find(' ');
        if (sp != std::string_view::npos) rest = rest.substr(0, sp);
        current_file->mode_changes.push_back({std::string(rest), ""});
      }
      continue;
    }
    if (line.starts_with("deleted file mode ")) {
      if (current_file) {
        current_file->type = DiffFileType::kDeletedFile;
        auto rest = line.substr(18);
        auto sp = rest.find(' ');
        if (sp != std::string_view::npos) rest = rest.substr(0, sp);
        current_file->mode_changes.push_back({std::string(rest), ""});
      }
      continue;
    }

    // --- / +++
    if (line.starts_with("--- ")) {
      if (!current_file) {
        // Pure unified diff without diff --git preamble: create a file.
        current_file = &model.files.emplace_back();
      }
      try_consume_old_file(line, *current_file);
      continue;
    }
    if (line.starts_with("+++ ")) {
      if (!current_file) {
        current_file = &model.files.emplace_back();
      }
      try_consume_new_file(line, *current_file);
      continue;
    }

    // rename / copy
    if (line.starts_with("rename from ")) {
      if (current_file) {
        current_file->old_path = std::string(line.substr(12));
        current_file->type = DiffFileType::kRenamedFile;
      }
      continue;
    }
    if (line.starts_with("rename to ")) {
      if (current_file) {
        current_file->new_path = std::string(line.substr(10));
      }
      continue;
    }
    if (line.starts_with("copy from ")) {
      if (current_file) {
        current_file->old_path = std::string(line.substr(10));
        current_file->type = DiffFileType::kCopyFile;
      }
      continue;
    }
    if (line.starts_with("copy to ")) {
      if (current_file) {
        current_file->new_path = std::string(line.substr(8));
      }
      continue;
    }

    // Binary files differ
    if (line.starts_with("Binary files ")) {
      if (current_file) {
        current_file->type = DiffFileType::kBinary;
        auto and_pos = line.find(" and ");
        if (and_pos != std::string_view::npos) {
          auto a_part = line.substr(12, and_pos - 12);
          auto b_part = line.substr(and_pos + 5);
          auto dpos = b_part.find(' ');
          if (dpos != std::string_view::npos) b_part = b_part.substr(0, dpos);
          auto slash_a = a_part.find('/');
          if (slash_a != std::string_view::npos)
            a_part.remove_prefix(slash_a + 1);
          auto slash_b = b_part.find('/');
          if (slash_b != std::string_view::npos)
            b_part.remove_prefix(slash_b + 1);
          current_file->old_path = std::string(a_part);
          current_file->new_path = std::string(b_part);
        }
      }
      continue;
    }

    // --- Hunk header ---
    if (line.starts_with("@@ ")) {
      auto hunk = try_parse_hunk_header(line);
      if (hunk.has_value() && current_file) {
        hunk->lines.push_back(
            {DiffChangeType::kHunkHeader, std::nullopt, std::nullopt,
             std::string(line)});
        current_hunk = &current_file->hunks.emplace_back(
            std::move(hunk.value()));
      }
      continue;
    }

    // --- Actual diff lines ---
    if (!line.empty() &&
        (line[0] == ' ' || line[0] == '+' || line[0] == '-')) {
      DiffLine dl;
      dl.change_type = classify_line_char(line[0]);
      dl.text = std::string(line.substr(1));

      if (current_hunk) {
        int old_idx = 0;
        int new_idx = 0;
        for (const auto& prev : current_hunk->lines) {
          if (prev.change_type == DiffChangeType::kContext ||
              prev.change_type == DiffChangeType::kDeletion) {
            old_idx++;
          }
          if (prev.change_type == DiffChangeType::kContext ||
              prev.change_type == DiffChangeType::kAddition) {
            new_idx++;
          }
        }
        if (dl.change_type == DiffChangeType::kDeletion ||
            dl.change_type == DiffChangeType::kContext) {
          dl.old_line_no = current_hunk->old_start + old_idx;
        }
        if (dl.change_type == DiffChangeType::kAddition ||
            dl.change_type == DiffChangeType::kContext) {
          dl.new_line_no = current_hunk->new_start + new_idx;
        }
      }

      if (current_hunk) {
        current_hunk->lines.push_back(std::move(dl));
      }
      continue;
    }

    // --- Context diff format: *** file *** / === file === ---
    // Heuristic: "*** 1,3 ****" (ends with " ****") is a section separator.
    // "*** file.txt  date" is a file header (ends with * after some content).
    bool is_context_diff_header = false;
    if (line.starts_with("*** ") && line.size() > 4) {
      // Check if this is a section separator: "*** 1,3 ****"
      bool is_separator = (line.size() >= 5 &&
          line.substr(line.size() - 5) == " ****");
      if (!is_separator) {
        // It's a file header like "*** file.txt  date"
        is_context_diff_header = true;
      }
    }
    if (line.starts_with("=== ") && line.size() > 4 && line.back() == '=') {
      is_context_diff_header = true;
    }

    if (is_context_diff_header) {
      current_file = &model.files.emplace_back();
      std::string_view content = line.substr(2);
      // Remove trailing " ****" if present (section separator)
      if (content.size() >= 5 && content.substr(content.size() - 5) == " ****") {
        content = content.substr(0, content.size() - 5);
      }
      // Remove trailing whitespace + timestamp (two+ spaces)
      auto ws = content.find("  ");
      if (ws != std::string_view::npos) content = content.substr(0, ws);
      // Strip leading path component
      auto slash = content.find('/');
      if (slash != std::string_view::npos)
        content = content.substr(slash + 1);
      current_file->old_path = std::string(content);
      current_file->new_path = current_file->old_path;
      current_hunk = nullptr;
      continue;
    }

    // --- Commit info ---
    if (line.starts_with("commit ")) {
      if (current_file && current_hunk) {
        current_hunk->lines.push_back(
            {DiffChangeType::kCommitInfo, std::nullopt, std::nullopt,
             std::string(line)});
      }
      continue;
    }

    // --- Mode lines ---
    if (line.starts_with("old mode ")) {
      if (current_file) {
        auto rest = line.substr(9);
        auto nl = rest.find('\n');
        if (nl != std::string_view::npos) rest = rest.substr(0, nl);
        current_file->mode_changes.emplace_back(std::string(rest), "");
      }
      continue;
    }
    if (line.starts_with("new mode ")) {
      if (current_file && !current_file->mode_changes.empty()) {
        auto rest = line.substr(9);
        auto nl = rest.find('\n');
        if (nl != std::string_view::npos) rest = rest.substr(0, nl);
        current_file->mode_changes.back().new_mode = std::string(rest);
      }
      continue;
    }
  }

  return model;
}

std::size_t DiffModel::hunk_count() const {
  std::size_t count = 0;
  for (const auto& f : files) count += f.hunks.size();
  return count;
}

std::size_t DiffModel::line_count() const {
  std::size_t count = 0;
  for (const auto& f : files)
    for (const auto& h : f.hunks) count += h.lines.size();
  return count;
}

std::size_t DiffModel::addition_count() const {
  std::size_t count = 0;
  for (const auto& f : files)
    for (const auto& h : f.hunks)
      for (const auto& l : h.lines)
        if (l.change_type == DiffChangeType::kAddition) count++;
  return count;
}

std::size_t DiffModel::deletion_count() const {
  std::size_t count = 0;
  for (const auto& f : files)
    for (const auto& h : f.hunks)
      for (const auto& l : h.lines)
        if (l.change_type == DiffChangeType::kDeletion) count++;
  return count;
}

std::size_t DiffModel::context_count() const {
  std::size_t count = 0;
  for (const auto& f : files)
    for (const auto& h : f.hunks)
      for (const auto& l : h.lines)
        if (l.change_type == DiffChangeType::kContext) count++;
  return count;
}

}  // namespace terminal_ui_kit
