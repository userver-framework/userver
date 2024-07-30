#pragma once

#include <string>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

enum class BitOperation { kAnd, kOr, kXor, kNot };

std::string ToString(BitOperation bitop);

}  // namespace storages::redis

USERVER_NAMESPACE_END
