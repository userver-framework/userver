#include <kafka/impl/stats.hpp>

#include <string_view>

#include <userver/utils/statistics/metadata.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

namespace {

constexpr std::string_view kSolomonLabel{"solomon_label"};

}  // namespace

void DumpMetric(utils::statistics::Writer& writer, const Stats& stats) {
  for (const auto& [topic, topic_stats] : stats.topics_stats) {
    const utils::statistics::LabelView label{kSolomonLabel, topic};

    writer[topic]["avg_ms_spent_time"].ValueWithLabels(
        topic_stats->avg_ms_spent_time.GetStatsForPeriod().GetCurrent().average,
        label);
    writer[topic]["messages_total"].ValueWithLabels(
        topic_stats->messages_counts.messages_total.Load(), label);
    writer[topic]["messages_success"].ValueWithLabels(
        topic_stats->messages_counts.messages_success.Load(), label);
    writer[topic]["messages_error"].ValueWithLabels(
        topic_stats->messages_counts.messages_error.Load(), label);
  }
  writer["connections_error"].ValueWithLabels(
      stats.connections_error.Load(), {kSolomonLabel, "component_name"});
}

}  // namespace kafka::impl

USERVER_NAMESPACE_END
