#include <kafka/impl/stats.hpp>

#include <userver/utils/statistics/metadata.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

formats::json::Value ExtendStatistics(const Stats& stats) {
  formats::json::ValueBuilder stats_builder(formats::json::Type::kObject);
  for (const auto& [topic, topic_stats] : stats.topics_stats) {
    stats_builder[topic]["avg_ms_spent_time"] =
        topic_stats->avg_ms_spent_time.GetStatsForPeriod().GetCurrent().average;
    stats_builder[topic]["messages_total"] =
        topic_stats->messages_counts.messages_total.Load();
    stats_builder[topic]["messages_success"] =
        topic_stats->messages_counts.messages_success.Load();
    stats_builder[topic]["messages_error"] =
        topic_stats->messages_counts.messages_error.Load();
    utils::statistics::SolomonLabelValue(stats_builder[topic], "topic");
  }
  stats_builder["connections_error"] = stats.connections_error.Load();
  utils::statistics::SolomonLabelValue(stats_builder, "component_name");
  return stats_builder.ExtractValue();
}

}  // namespace kafka::impl

USERVER_NAMESPACE_END
