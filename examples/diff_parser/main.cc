// Example: UnifiedDiffParser
//
// Reads a unified diff (as produced by `git diff --no-color`) from standard
// input and prints a plain-text summary of the parsed model: file sections,
// hunk headers, and each line with its old/new line numbers and role.
//
// This is intentionally a non-interactive, terminal-agnostic demo of the
// parser/model only; rendering and navigation are separate concerns handled
// by the view layer.
//
// Usage:
//   git diff --no-color | terminal_ui_kit_example_diff_parser
//   terminal_ui_kit_example_diff_parser < changes.diff

#include <iostream>
#include <string>

#include "terminal_ui_kit/diff/unified_diff_parser.h"

using terminal_ui_kit::diff::DiffFile;
using terminal_ui_kit::diff::DiffHunk;
using terminal_ui_kit::diff::DiffLine;
using terminal_ui_kit::diff::DiffLineType;
using terminal_ui_kit::diff::UnifiedDiffParser;

namespace {

const char* TypeName(DiffLineType type) {
  switch (type) {
    case DiffLineType::kContext:
      return "context";
    case DiffLineType::kAdded:
      return "added  ";
    case DiffLineType::kDeleted:
      return "deleted";
  }
  return "?";
}

std::string PlainText(const terminal_ui_kit::StyledText& text) {
  std::string result;
  for (const auto& span : text.spans()) result += span.text;
  return result;
}

}  // namespace

int main() {
  std::string input;
  std::string line;
  while (std::getline(std::cin, line)) {
    input += line;
    input.push_back('\n');
  }

  const std::vector<DiffFile> files = UnifiedDiffParser{}.Parse(input);
  std::cout << files.size() << " file section(s) parsed.\n";

  for (const DiffFile& file : files) {
    std::cout << "--- " << file.old_path << "  +++ " << file.new_path << "\n";
    std::cout << "    " << file.hunks.size() << " hunk(s)\n";
    for (const DiffHunk& hunk : file.hunks) {
      std::cout << hunk.header << "\n";
      for (const DiffLine& entry : hunk.lines) {
        const std::string old_no = entry.old_line ? std::to_string(*entry.old_line) : " ";
        const std::string new_no = entry.new_line ? std::to_string(*entry.new_line) : " ";
        std::cout << "  " << old_no << " " << new_no << " " << TypeName(entry.type) << " | "
                  << PlainText(entry.content) << "\n";
      }
    }
  }
  return 0;
}
