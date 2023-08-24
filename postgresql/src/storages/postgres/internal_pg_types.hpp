#pragma once

#include <cstdint>

#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {
class LogHelper;
}  // namespace logging

namespace storages::postgres {

using Lsn = USERVER_NAMESPACE::utils::StrongTypedef<struct LsnTag, uint64_t>;
constexpr Lsn kUnknownLsn{0};

logging::LogHelper& operator<<(logging::LogHelper& lh, Lsn lsn);

}  // namespace storages::postgres

USERVER_NAMESPACE_END
