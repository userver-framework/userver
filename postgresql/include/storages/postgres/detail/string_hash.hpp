#pragma once

#include <cstdint>

#include <utils/string_view.hpp>

namespace storages::postgres::utils {

std::size_t StrHash(const char* str, std::size_t len);
inline std::size_t StrHash(const ::utils::string_view& str) {
  return StrHash(str.data(), str.size());
}

struct StringViewHash {
  std::size_t operator()(const ::utils::string_view& str) const {
    return StrHash(str);
  }
};

}  // namespace storages::postgres::utils
