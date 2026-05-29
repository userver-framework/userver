#pragma once

#include <cstdint>

#include <userver/utils/statistics/histogram.hpp>
#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/utils/statistics/relaxed_counter.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

// A reduced set of buckets for by-query metrics.
// We try to spare the service's metrics quota by default.
constexpr inline double kDefaultHistogramBoundsArray[] =
    {5, 10, 20, 35, 60, 100, 173, 300, 520, 1000, 3200, 10000, 32000, 100000};

struct TransactionStatistics final {
    utils::statistics::RateCounter total{};
    utils::statistics::RateCounter commit_total{};
    utils::statistics::RateCounter rollback_total{};
    utils::statistics::RateCounter out_of_trx_total{};
    utils::statistics::RateCounter execute_total{};
    utils::statistics::RateCounter error_execute_total{};
    utils::statistics::RateCounter execute_timeout{};

    utils::statistics::Histogram total_percentile{kDefaultHistogramBoundsArray};
    utils::statistics::Histogram busy_percentile{kDefaultHistogramBoundsArray};
};

struct ConnectionStatistics final {
    using GaugeMetric = utils::statistics::RelaxedCounter<std::uint64_t>;

    utils::statistics::RateCounter open_total{};
    utils::statistics::RateCounter drop_total{};
    GaugeMetric active{};
    GaugeMetric used{};
    GaugeMetric maximum{};
    GaugeMetric waiting{};
    utils::statistics::RateCounter error_total{};
    utils::statistics::RateCounter error_timeout{};
};

struct InstanceStatistics final {
    ConnectionStatistics connection{};
    TransactionStatistics transaction{};
    utils::statistics::RateCounter pool_exhaust_errors{};

    utils::statistics::Histogram connection_percentile{kDefaultHistogramBoundsArray};
    utils::statistics::Histogram acquire_percentile{kDefaultHistogramBoundsArray};
};

void DumpMetric(utils::statistics::Writer& writer, const InstanceStatistics& stats);

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
