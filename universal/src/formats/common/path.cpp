#include <userver/formats/common/path.hpp>

#include <array>

#include <fmt/compile.h>
#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace formats::common {

namespace {

constexpr const size_t kIndexCacheSize = 1024;

const std::array<std::string, kIndexCacheSize> kIndexCache = [] {
  std::array<std::string, kIndexCacheSize> arr{};
  for (size_t i = 0; i < kIndexCacheSize; i++) {
    arr[i] = GetIndexString(i);
  }
  return arr;
}();

}  // namespace

void AppendPath(std::string& path, std::string_view key) {
  if (!path.empty() && path.back() != '/') path += kPathSeparator;
  path += key;
}

void AppendPath(std::string& path, std::size_t index) {
  if (index < kIndexCacheSize) {
    path += kIndexCache[index];
  } else {
    fmt::format_to(std::back_inserter(path), FMT_COMPILE("[{}]"), index);
  }
}

std::string MakeChildPath(std::string_view parent, std::string_view key) {
  if (parent.empty() || parent == kPathRoot) return std::string{key};

  std::string new_path{parent};
  AppendPath(new_path, key);
  return new_path;
}

std::string MakeChildPath(std::string_view parent, std::size_t index) {
  std::string new_path;
  if (!parent.empty() && parent != kPathRoot) new_path = parent;

  AppendPath(new_path, index);
  return new_path;
}

std::string GetIndexString(size_t index) {
  return fmt::format(FMT_COMPILE("[{}]"), index);
}

Path::Path() = default;

Path::Path(std::string path) : path_(std::move(path)) {}

bool Path::IsRoot() const { return path_.empty(); }

std::string Path::ToString() const { return IsRoot() ? kPathRoot : path_; }

std::string_view Path::ToStringView() const {
  return IsRoot() ? kPathRoot : std::string_view{path_};
}

Path Path::MakeChildPath(std::string_view key) const {
  return Path(formats::common::MakeChildPath(path_, key));
}

Path Path::MakeChildPath(std::size_t index) const {
  return Path(formats::common::MakeChildPath(path_, index));
}

Path Path::WithPrefix(std::string path_prefix) {
  Path result;
  result.path_ = std::move(path_prefix);
  result.path_ += kPathPrefixSeparator;
  return result;
}

}  // namespace formats::common

USERVER_NAMESPACE_END
