#pragma once

/// @file userver/storages/redis/bit_operation.hpp
/// @brief @copybrief storages::redis::BitOperation

#include <string>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

/// @brief Bitwise SET operation kind for Redis BITOP
enum class BitOperation { kAnd, kOr, kXor, kNot };

std::string ToString(BitOperation bitop);

}  // namespace storages::redis

USERVER_NAMESPACE_END
