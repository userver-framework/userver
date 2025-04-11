#include <userver/kafka/impl/stats.hpp>

#include <string_view>

#include <userver/utils/statistics/metadata.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

namespace {

constexpr std::string_view kTopic{"topic"};
constexpr std::string_view kComponentName{"component_name"};

}  // namespace

void DumpMetric(utils::statistics::Writer& writer, const Stats& stats, const std::string& component_name) {
    const utils::statistics::LabelView component_label{kComponentName, component_name};
    for (const auto& [topic, topic_stats] : stats.topics_stats) {
        const utils::statistics::LabelView topic_label{kTopic, topic};

        writer["avg_ms_spent_time"].ValueWithLabels(
            topic_stats->avg_ms_spent_time.GetStatsForPeriod().GetCurrent().average, {component_label, topic_label}
        );
        writer["messages_total"].ValueWithLabels(
            topic_stats->messages_counts.messages_total.Load(), {component_label, topic_label}
        );
        writer["messages_success"].ValueWithLabels(
            topic_stats->messages_counts.messages_success.Load(), {component_label, topic_label}
        );
        writer["messages_error"].ValueWithLabels(
            topic_stats->messages_counts.messages_error.Load(), {component_label, topic_label}
        );
    }
    writer["connections_error"].ValueWithLabels(stats.connections_error.Load(), component_label);
}

}  // namespace kafka::impl

USERVER_NAMESPACE_END
