#include <formats/common/path_impl.hpp>

namespace formats::common::impl {
std::string MakeChildPath(const std::string& parent, const std::string& key) {
  if (parent.empty() || parent == kPathRoot) {
    return key;
  } else {
    std::string new_path;
    new_path.reserve(parent.size() + sizeof(kPathSeparator) + key.size());
    new_path += parent;
    new_path += kPathSeparator;
    new_path += key;
    return new_path;
  }
}

std::string MakeChildPath(const std::string& parent, std::size_t index) {
  std::string new_path;
  const auto ind = std::to_string(index);

  constexpr static std::size_t kIndexDecorationSize = 2;
  if (parent.empty() || parent == kPathRoot) {
    new_path.reserve(kIndexDecorationSize + ind.size());
  } else {
    new_path.reserve(parent.size() + sizeof(kPathSeparator) +
                     kIndexDecorationSize + ind.size());
    new_path += parent;
    new_path += kPathSeparator;
  }

  new_path += '[';
  new_path += ind;
  new_path += ']';

  return new_path;
}
}  // namespace formats::common::impl
