#pragma once

#include <cstdint>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl::dist_lock {

inline constexpr std::uint64_t kSemaphoreLimit = 1;

}  // namespace ydb::impl::dist_lock

USERVER_NAMESPACE_END
