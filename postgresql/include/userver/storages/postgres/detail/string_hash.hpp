#pragma once

#include <cstdint>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::utils {

std::size_t StrHash(const char* str, std::size_t len);
inline std::size_t StrHash(const std::string_view& str) {
  return StrHash(str.data(), str.size());
}

struct StringViewHash {
  std::size_t operator()(const std::string_view& str) const {
    return StrHash(str);
  }
};

}  // namespace storages::postgres::utils

USERVER_NAMESPACE_END
