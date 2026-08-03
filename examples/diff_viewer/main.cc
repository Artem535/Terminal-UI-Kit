#include <cstddef>
#include <iostream>
#include <string>

#include "terminal_ui_kit/diff/unified_diff_parser.h"

namespace {

const char* DiffLineTypeName(terminal_ui_kit::DiffLineType type) {
  using terminal_ui_kit::DiffLineType;
  switch (type) {
    case DiffLineType::kContext:
      return "context";
    case DiffLineType::kAdded:
      return "added";
    case DiffLineType::kRemoved:
      return "removed";
  }
  return "unknown";
}

void PrintDiffResult(const terminal_ui_kit::UnifiedDiffResult& result) {
  if (!result.success) {
    std::cout << "Parse error: " << result.error_message << "\n";
    return;
  }

  if (result.files.empty()) {
    std::cout << "No files in diff.\n";
    return;
  }

  for (std::size_t i = 0; i < result.files.size(); ++i) {
    const auto& file = result.files[i];
    std::cout << "File " << (i + 1) << "/" << result.files.size() << ":\n";
    std::cout << "  old_path: " << file.old_path << "\n";
    std::cout << "  new_path: " << file.new_path << "\n";
    std::cout << "  binary: " << (file.is_binary ? "yes" : "no") << "\n";
    std::cout << "  old_no_newline: " << (file.old_no_newline ? "yes" : "no") << "\n";
    std::cout << "  new_no_newline: " << (file.new_no_newline ? "yes" : "no") << "\n";

    if (file.is_binary) {
      continue;
    }

    std::cout << "  hunks: " << file.hunks.size() << "\n";
    for (const auto& hunk : file.hunks) {
      std::cout << "    Hunk @@ -" << hunk.old_start << "," << hunk.old_count << " +"
                << hunk.new_start << "," << hunk.new_count;
      if (!hunk.context.empty()) {
        std::cout << " " << hunk.context;
      }
      std::cout << "\n";
      for (const auto& line : hunk.lines) {
        std::cout << "      [" << DiffLineTypeName(line.type) << "]";
        if (line.old_line.has_value()) {
          std::cout << " old:" << line.old_line.value();
        }
        if (line.new_line.has_value()) {
          std::cout << " new:" << line.new_line.value();
        }
        std::cout << " \"" << line.text << "\"\n";
      }
    }
  }
}

}  // namespace

int main() {
  const std::string sample_diff = R"(diff --git a/hello.txt b/hello.txt
index 1234567..abcdefg 100644
--- a/hello.txt
+++ b/hello.txt
@@ -1,4 +1,4 @@
 line one
 line two
-line three
+line three updated
 line four
diff --git a/world.txt b/world.txt
new file mode 100644
--- /dev/null
+++ b/world.txt
@@ -0,0 +1,2 @@
+first
+second
diff --git a/binary.png b/binary.png
index abcdefg..1234567
Binary files a/binary.png and b/binary.png differ
)";

  const auto result = terminal_ui_kit::UnifiedDiffParser::Parse(sample_diff);
  PrintDiffResult(result);

  return 0;
}
