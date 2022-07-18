#pragma once

#include <cstdint>
#include <limits>

#include <fmt/format.h>

#include <userver/formats/bson/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::bson::impl {

inline int64_t ToInt64(uint64_t value) {
  if (value > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
    throw BsonException(fmt::format("Value {} is too large for BSON", value));
  }
  return static_cast<int64_t>(value);
}

}  // namespace formats::bson::impl

USERVER_NAMESPACE_END
