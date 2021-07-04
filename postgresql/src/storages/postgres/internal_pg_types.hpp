#pragma once

#include <cstdint>

#include <userver/utils/strong_typedef.hpp>

namespace logging {
class LogHelper;
}  // namespace logging

namespace storages::postgres {

using Lsn = ::utils::StrongTypedef<struct LsnTag, uint64_t>;
constexpr Lsn kUnknownLsn{0};

logging::LogHelper& operator<<(logging::LogHelper& lh, Lsn lsn);

}  // namespace storages::postgres
