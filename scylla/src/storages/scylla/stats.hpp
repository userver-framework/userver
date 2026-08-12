#pragma once

#include <array>
#include <chrono>
#include <exception>
#include <string_view>

#include <userver/utils/not_null.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/statistics/percentile.hpp>
#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/utils/statistics/recentperiod.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::stats {

using Rate = utils::statistics::Rate;
using Counter = utils::statistics::RateCounter;
using TimingsPercentile = utils::statistics::Percentile<1000, uint32_t, 780, 50>;
using AggregatedTimingsPercentile = utils::statistics::RecentPeriod<TimingsPercentile, TimingsPercentile>;

enum class ErrorType : std::size_t {
    kSuccess,

    kNetwork,
    kClusterUnavailable,
    kAuthFailure,
    kBadQueryArgument,
    kTimeout,
    kServerError,
    kOther,

    kErrorTypesCount,
};

inline constexpr auto kErrorTypesCount = static_cast<std::size_t>(ErrorType::kErrorTypesCount);

std::string_view ToString(ErrorType type);

ErrorType ClassifyException(const std::exception& ex) noexcept;

struct OperationStatisticsItem final {
    void Account(ErrorType error_type, std::chrono::milliseconds elapsed) noexcept;

    Rate GetCounter(ErrorType error_type) const noexcept;
    Rate GetTotalQueries() const noexcept;

    std::array<Counter, kErrorTypesCount> counters;
    Counter timings_sum{0};
    AggregatedTimingsPercentile timings;
};

void DumpMetric(utils::statistics::Writer& writer, const OperationStatisticsItem& item);

struct SessionStatistics final {
    SessionStatistics()
        : ping(utils::MakeSharedRef<OperationStatisticsItem>()),
          queries(utils::MakeSharedRef<OperationStatisticsItem>())
    {}

    Counter created;
    Counter closed;
    Counter reconnects;

    utils::SharedRef<OperationStatisticsItem> ping;
    utils::SharedRef<OperationStatisticsItem> queries;
};

void DumpMetric(utils::statistics::Writer& writer, const SessionStatistics& stats);

struct ScyllaSessionStatistics final {
    ScyllaSessionStatistics()
        : session(utils::MakeSharedRef<SessionStatistics>())
    {}

    utils::SharedRef<SessionStatistics> session;
};

}  // namespace storages::scylla::stats

USERVER_NAMESPACE_END
