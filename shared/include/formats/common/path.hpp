#pragma once

/// @file formats/common/path.hpp
/// @brief @copybrief formats::common::Path

#include <string>

namespace formats::common {

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
