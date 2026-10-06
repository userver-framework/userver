#include <clients/http/destination_statistics.hpp>

#include <vector>

#include <userver/logging/log.hpp>
#include <userver/server/request/impl/client_metrics_shard.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

namespace {

constexpr std::string_view kHttpDestinationLabel = "http_destination";

}  // namespace

RequestStats DestinationStatistics::GetStatisticsForDestination(std::string_view destination) {
    return RequestStats{metrics_[{.http_destination = destination}]};
}

std::optional<RequestStats> DestinationStatistics::GetStatisticsForDestinationAuto(std::string_view destination) {
    auto* const stats = metrics_.GetIfExists({.http_destination = destination});
    if (stats) {
        return std::optional<RequestStats>{std::in_place, *stats};
    }

    // atomic [current++ iff current < max]
    auto current_auto_destinations = current_auto_destinations_.load();
    do {
        if (current_auto_destinations >= max_auto_destinations_) {
            if (max_auto_destinations_ != 0) {
                LOG_LIMITED_WARNING()
                    << "Too many httpclient metrics destinations used (" << max_auto_destinations_
                    << "), either increase "
                       "components.http-client-core.destination-metrics-auto-max-size "
                       "or explicitly set destination via "
                       "Request::SetDestinationMetricName().";
            }
            return {};
        }
    } while (!current_auto_destinations_
                  .compare_exchange_strong(current_auto_destinations, current_auto_destinations + 1));

    return GetStatisticsForDestination(destination);
}

std::optional<RequestStats> DestinationStatistics::GetShardedStatisticsForDestination(std::string_view destination) {
    const auto* const shard = server::request::impl::kClientMetricsShard.GetOptional();
    if (!shard) {
        return std::nullopt;
    }

    std::vector<utils::statistics::LabelView> labels;
    labels.reserve(shard->labels.size() + 1);
    labels.emplace_back(kHttpDestinationLabel, destination);
    for (const auto& label : shard->labels) {
        labels.emplace_back(label);
    }

    return std::optional<RequestStats>{std::in_place, sharded_metrics_.TryEmplace({shard->path, labels}).first};
}

void DestinationStatistics::SetAutoMaxSize(size_t max_auto_destinations) {
    max_auto_destinations_ = max_auto_destinations;
}

void DestinationStatistics::VisitAllDebug(utils::function_ref<void(const DestinationLabels&, const Statistics&)> func
) const {
    metrics_.VisitAll(func);
}

void DumpMetric(utils::statistics::Writer& writer, const DestinationStatistics& stats) {
    stats.metrics_.VisitAll([&writer](const DestinationLabels& labels, const Statistics& destination_stats) {
        const InstanceStatistics instance_stat{destination_stats};
        writer.ValueWithLabels(
            DestinationStatisticsView{instance_stat},
            {{kHttpDestinationLabel, labels.http_destination}, {"version", "2"}}
        );
    });

    stats.sharded_metrics_
        .Visit([&writer](const utils::statistics::impl::StatisticsKey& key, const Statistics& sharded_stats) {
            const InstanceStatistics instance_stat{sharded_stats};
            auto labels = key.label_views;
            labels.emplace_back("version", "2");
            writer[key.path].ValueWithLabels(ShardedDestinationStatisticsView{instance_stat}, labels);
        });
}

}  // namespace clients::http

USERVER_NAMESPACE_END
