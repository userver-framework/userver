#pragma once

#include <chrono>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

using Rtt = std::chrono::microseconds;
inline constexpr Rtt kUnknownRtt{-1};

void UpdateAverageRtt(Rtt& average_rtt, Rtt latest_rtt) noexcept;

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
