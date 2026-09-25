#pragma once

#include <atomic>
#include <cstddef>
#include <optional>
#include <string_view>

#include <userver/utils/function_ref.hpp>
#include <userver/utils/required.hpp>
#include <userver/utils/statistics/by_label_storage.hpp>
#include <userver/utils/statistics/fwd.hpp>

#include <clients/http/statistics.hpp>
#include <utils/statistics/impl/monotonic_concurrent_statistics_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

struct DestinationLabels final {
    utils::Required<std::string_view> http_destination;
};

class DestinationStatistics final {
public:
    // It's safe to return RequestStats holding a reference to Statistics as
    // RequestStats lifetime is less than clients::http::Client's one.
    RequestStats GetStatisticsForDestination(std::string_view destination);

    // If max_auto_destinations is reached, returns nullopt.
    std::optional<RequestStats> GetStatisticsForDestinationAuto(std::string_view destination);

    // Returns nullopt if server::request::SetClientMetricsShard was not called in the current task hierarchy.
    std::optional<RequestStats> GetShardedStatisticsForDestination(std::string_view destination);

    void SetAutoMaxSize(size_t max_auto_destinations);

    void VisitAllDebug(utils::function_ref<void(const DestinationLabels&, const Statistics&)> func) const;

    friend void DumpMetric(utils::statistics::Writer& writer, const DestinationStatistics& stats);

private:
    utils::statistics::MonotonicByLabelStorage<DestinationLabels, Statistics> metrics_;
    utils::statistics::impl::MonotonicConcurrentStatisticsMap<Statistics> sharded_metrics_;
    std::size_t max_auto_destinations_{0};
    std::atomic<std::size_t> current_auto_destinations_{0};
};

void DumpMetric(utils::statistics::Writer& writer, const DestinationStatistics& stats);

}  // namespace clients::http

USERVER_NAMESPACE_END
