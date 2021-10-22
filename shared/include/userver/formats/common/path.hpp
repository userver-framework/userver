#pragma once

/// @file userver/formats/common/path.hpp
/// @brief @copybrief formats::common::Path

#include <string>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace formats::common {

inline constexpr char kPathSeparator = '.';
inline constexpr char kPathRoot[] = "/";
inline constexpr char kPathPrefixSeparator[] = ":/";

/// Returns string of [idx], e.g. "[0]" or "[1025]"
std::string GetIndexString(size_t index);

void AppendPath(std::string& path, std::string_view key);
void AppendPath(std::string& path, std::size_t index);

std::string MakeChildPath(std::string_view parent, std::string_view key);
std::string MakeChildPath(std::string_view parent, std::size_t index);

/// Document/array element path storage
class Path {
 public:
  Path();

  bool IsRoot() const;
  std::string ToString() const;
  std::string_view ToStringView() const;

  Path MakeChildPath(std::string_view key) const;
  Path MakeChildPath(std::size_t index) const;

  static Path WithPrefix(std::string path_prefix);

 private:
  explicit Path(std::string path);

  std::string path_;
};

}  // namespace formats::common

USERVER_NAMESPACE_END
