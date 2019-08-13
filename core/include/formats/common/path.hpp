#pragma once

/// @file formats/common/path.hpp
/// @brief @copybrief formats::common::Path

#include <string>
#include <vector>

namespace formats::common {

/// Document/array element path storage
class Path {
 public:
  std::string ToString() const;

  Path MakeChildPath(const std::string& key) const;
  Path MakeChildPath(std::size_t index) const;

 private:
  std::string path_;
};

}  // namespace formats::common
