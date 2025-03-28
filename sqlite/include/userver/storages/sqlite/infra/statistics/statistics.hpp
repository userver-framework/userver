#pragma once

#include <cstdint>
#include <string>

#include <userver/utils/statistics/percentile.hpp>
#include <userver/utils/statistics/recentperiod.hpp>
#include <userver/utils/statistics/relaxed_counter.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra::statistics {

using Counter = utils::statistics::RelaxedCounter<std::uint64_t>;
using Percentile = utils::statistics::Percentile<2048, uint64_t, 16, 256>;
using RecentPeriod = utils::statistics::RecentPeriod<Percentile, Percentile>;

struct PoolConnectionStatistics final {
  std::string type;  // connection pool type: write / read

  Counter overload{};
  Counter closed{};
  Counter created{};
  Counter acquired{};
  Counter released{};
};

struct PoolQueryStatistics final {
  Counter total{};
  Counter error{};
  RecentPeriod timings{};
};

struct PoolTransactionStatistics final {
  Counter total{};
  Counter commit_total{};
  Counter rollback_total{};
};

struct PoolStatistics final {
  PoolTransactionStatistics transactions{};
  PoolQueryStatistics write_operations{};
  PoolQueryStatistics read_operations{};
  PoolConnectionStatistics connections{};
};

void DumpMetric(utils::statistics::Writer& writer, const PoolStatistics& stats);

void DumpMetric(utils::statistics::Writer& writer,
                const PoolQueryStatistics& stats);

void DumpMetric(utils::statistics::Writer& writer,
                const PoolConnectionStatistics& stats);

void DumpMetric(utils::statistics::Writer& writer,
                const PoolTransactionStatistics& stats);

}  // namespace storages::sqlite::infra::statistics

USERVER_NAMESPACE_END
