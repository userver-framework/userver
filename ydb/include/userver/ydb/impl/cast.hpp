#pragma once

#include <string>
#include <type_traits>

#include "string.hpp"

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

inline decltype(auto) ToString(const std::string& s) {
  if constexpr (std::is_same_v<impl::String, std::string>) {
    return s;
  } else {
    return String{s};
  }
}

inline String ToString(const std::string_view& s) { return String{s}; }

}  // namespace ydb::impl

USERVER_NAMESPACE_END
