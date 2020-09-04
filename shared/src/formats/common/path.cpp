#include <formats/common/path.hpp>

#include <fmt/format.h>

#include <formats/common/path_impl.hpp>

namespace formats::common {

const std::array<std::string, kIndexCacheSize> kIndexCache = [] {
  std::array<std::string, kIndexCacheSize> arr;
  for (size_t i = 0; i < std::size(arr); i++) arr[i] = GetIndexString(i);
  return arr;
}();

std::string GetIndexString(size_t index) {
  return fmt::format(FMT_STRING("[{}]"), index);
}

Path::Path() = default;

Path::Path(std::string path) : path_(std::move(path)) {}

std::string Path::ToString() const {
  if (path_.empty()) {
    return impl::kPathRoot;
  }

  return path_;
}

Path Path::MakeChildPath(const std::string& key) const {
  return Path(impl::MakeChildPath(path_, key));
}

Path Path::MakeChildPath(std::size_t index) const {
  return Path(impl::MakeChildPath(path_, index));
}

}  // namespace formats::common
