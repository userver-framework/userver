#include <formats/common/path.hpp>

#include <formats/common/path_impl.hpp>

namespace formats::common {

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
