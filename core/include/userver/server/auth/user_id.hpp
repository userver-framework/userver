#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

#include <userver/formats/parse/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {
class LogHelper;
}

namespace server::auth {

/// Type that represents a yandex uid and has all the comparison and hashing
/// operators. It is not implicitly convertible to/from integers and provides
/// better type safety.
enum class UserId : std::uint64_t {};

using UserIds = std::vector<UserId>;

template <class Value>
UserId Parse(const Value& v, formats::parse::To<UserId>) {
  return UserId{v.template As<std::uint64_t>()};
}

inline std::uint64_t ToUInt64(UserId v) noexcept {
  return static_cast<std::uint64_t>(v);
}

std::string ToString(UserId v);

template <class Char>
std::basic_ostream<Char>& operator<<(std::basic_ostream<Char>& os, UserId v) {
  return os << ToUInt64(v);
}

// Optimized output for LogHelper
logging::LogHelper& operator<<(logging::LogHelper& os, UserId v);

}  // namespace server::auth

USERVER_NAMESPACE_END
