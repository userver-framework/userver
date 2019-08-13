#include <formats/common/path.hpp>

namespace formats::common {

constexpr char kPathSeparator = '.';
constexpr char kPathRoot[] = "/";

std::string Path::ToString() const {
  if (path_.empty()) {
    return kPathRoot;
  }

  return path_;
}

Path Path::MakeChildPath(const std::string& key) const {
  Path new_path;

  if (!path_.empty()) {
    new_path.path_.reserve(path_.size() + sizeof(kPathSeparator) + key.size());
    new_path.path_ += path_;
    new_path.path_ += kPathSeparator;
  }
  new_path.path_ += key;

  return new_path;
}

Path Path::MakeChildPath(std::size_t index) const {
  Path new_path;

  const auto ind = std::to_string(index);

  constexpr static std::size_t kIndexDecorationSize = 2;
  if (!path_.empty()) {
    new_path.path_.reserve(path_.size() + kIndexDecorationSize +
                           sizeof(kPathSeparator) + ind.size());
    new_path.path_ += path_;
    new_path.path_ += kPathSeparator;
  } else {
    new_path.path_.reserve(kIndexDecorationSize + ind.size());
  }

  new_path.path_ += '[';
  new_path.path_ += ind;
  new_path.path_ += ']';

  return new_path;
}

}  // namespace formats::common
