#include "subscription_statistics.hpp"

USERVER_NAMESPACE_BEGIN

namespace redis {

void DumpMetric(utils::statistics::Writer& writer,
                const PubsubChannelStatistics& stats) {
  writer["messages"]["count"] = stats.messages_count;
  writer["messages"]["alien-count"] = stats.messages_alien_count;
  writer["messages"]["size"] = stats.messages_size;

  if (stats.server_id) {
    auto diff = std::chrono::steady_clock::now() - stats.subscription_timestamp;
    writer["subscribed-ms"] =
        std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();

    auto inst_name = stats.server_id->GetDescription();
    if (inst_name.empty()) inst_name = "unknown";
    writer.ValueWithLabels(1, {"redis_instance", inst_name});
  }
}

void DumpMetric(utils::statistics::Writer& writer,
                const PubsubShardStatistics& stats) {
  for (const auto& [name, channel_stats] : stats.by_channel) {
    writer.ValueWithLabels(channel_stats, {"redis_pubsub_channel", name});
  }
}

void DumpMetric(utils::statistics::Writer& writer,
                const PubsubClusterStatistics& stats) {
  if (stats.settings.per_shard_stats_enabled) {
    for (const auto& [name, shard_stats] : stats.by_shard) {
      writer.ValueWithLabels(shard_stats, {"redis_shard", name});
    }
  }
  writer = stats.SumByShards();
}

}  // namespace redis

USERVER_NAMESPACE_END
