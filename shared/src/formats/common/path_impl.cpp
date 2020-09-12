#include <formats/common/path_impl.hpp>

#include <iterator>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <formats/common/path.hpp>

namespace formats::common::impl {

void AppendPath(std::string& path, std::string_view key) {
  if (!path.empty()) path += kPathSeparator;
  path += key;
}

void AppendPath(std::string& path, std::size_t index) {
  if (index < kIndexCacheSize) {
    path += formats::common::kIndexCache[index];
  } else {
    // TODO: replace with FMT_COMPILE
    fmt::format_to(std::back_inserter(path), "[{}]", index);
  }
}

std::string MakeChildPath(const std::string& parent, std::string_view key) {
  if (parent.empty() || parent == kPathRoot) return std::string{key};

  std::string new_path{parent};
  AppendPath(new_path, key);
  return new_path;
}

std::string MakeChildPath(const std::string& parent, std::size_t index) {
  std::string new_path;
  if (!parent.empty() && parent != kPathRoot) new_path = parent;

  AppendPath(new_path, index);
  return new_path;
}
}  // namespace formats::common::impl
