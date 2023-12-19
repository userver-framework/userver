#pragma once

#include <array>
#include <atomic>

#include <userver/logging/level.hpp>
#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::statistics {

using Counter = utils::statistics::RateCounter;

struct LogStatistics final {
  Counter dropped{};

  std::array<Counter, kLevelMax + 1> by_level{};
  std::atomic<bool> has_reopening_error{false};
};

void DumpMetric(utils::statistics::Writer& writer, const LogStatistics& stats);

}  // namespace logging::statistics

USERVER_NAMESPACE_END
