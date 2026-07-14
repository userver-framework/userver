#pragma once

#include <cstdint>

#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/utils/statistics/relaxed_counter.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::infra {

using Counter = utils::statistics::RelaxedCounter<std::uint64_t>;
using RateCounter = utils::statistics::RateCounter;

struct PoolConnectionStatistics final {
    // Monotonic event counters, reported as RATE metrics.
    RateCounter overload{};
    RateCounter closed{};
    RateCounter created{};

    // Used only to derive the instantaneous "busy" gauge below.
    Counter acquired{};
    Counter released{};
};

void DumpMetric(utils::statistics::Writer& writer, const PoolConnectionStatistics& stats);

}  // namespace storages::mysql::infra

USERVER_NAMESPACE_END
