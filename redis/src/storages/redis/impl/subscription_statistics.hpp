#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <userver/storages/redis/impl/base.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

struct PubsubChannelStatistics {
  std::chrono::steady_clock::time_point subscription_timestamp;
  size_t messages_count{0};
  size_t messages_size{0};
  size_t messages_alien_count{0};

  std::optional<ServerId> server_id;

  void AccountMessage(size_t message_size) {
    messages_count++;
    messages_size += message_size;
  }

  void AccountAlienMessage() { messages_alien_count++; }

  PubsubChannelStatistics& operator+=(const PubsubChannelStatistics& other) {
    subscription_timestamp = std::chrono::steady_clock::time_point();
    messages_count += other.messages_count;
    messages_size += other.messages_size;
    messages_alien_count += other.messages_alien_count;
    return *this;
  }
};

struct PubsubShardStatistics {
  std::unordered_map<std::string, PubsubChannelStatistics> by_channel;
};

struct RawPubsubClusterStatistics {
  std::vector<PubsubShardStatistics> by_shard;
};

struct PubsubClusterStatistics {
  PubsubClusterStatistics(const PubsubMetricsSettings& settings)
      : settings(settings) {}

  const PubsubMetricsSettings& settings;
  std::unordered_map<std::string, PubsubShardStatistics> by_shard;

  PubsubShardStatistics SumByShards() const {
    PubsubShardStatistics sum;
    for (const auto& shard : by_shard) {
      for (const auto& it : shard.second.by_channel)
        sum.by_channel[it.first] += it.second;
    }
    return sum;
  }
};

void DumpMetric(utils::statistics::Writer& writer,
                const PubsubChannelStatistics& stats);

void DumpMetric(utils::statistics::Writer& writer,
                const PubsubShardStatistics& stats);

void DumpMetric(utils::statistics::Writer& writer,
                const PubsubClusterStatistics& stats);

}  // namespace redis

USERVER_NAMESPACE_END
