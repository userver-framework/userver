#pragma once

/// @file formats/common/path.hpp
/// @brief @copybrief formats::common::Path

#include <array>
#include <string>

namespace formats::common {

/// Returns string of [idx], e.g. "[0]" or "[1024]"
std::string GetIndexString(size_t index);

constexpr const size_t kIndexCacheSize = 1024;

/// Cache of GetIndexString() for small numbers [0; kIndexCacheSize)
extern const std::array<std::string, kIndexCacheSize> kIndexCache;

/// Document/array element path storage
class Path {
 public:
  Path();

  std::string ToString() const;

  Path MakeChildPath(const std::string& key) const;
  Path MakeChildPath(std::size_t index) const;

 private:
  explicit Path(std::string path);
  std::string path_;
};

}  // namespace formats::common
