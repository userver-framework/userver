#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <userver/storages/redis/base.hpp>
#include <userver/utils/statistics/rate.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

struct PubsubChannelStatistics {
    std::chrono::steady_clock::time_point subscription_timestamp;
    USERVER_NAMESPACE::utils::statistics::Rate messages_count;
    USERVER_NAMESPACE::utils::statistics::Rate messages_size;
    USERVER_NAMESPACE::utils::statistics::Rate messages_alien_count;
    // messages that were discarded by subscription queue because it
    // overflowed
    USERVER_NAMESPACE::utils::statistics::Rate messages_discarded;

    std::optional<ServerId> server_id;

    void AccountMessage(size_t message_size) {
        messages_count += USERVER_NAMESPACE::utils::statistics::Rate{1};
        messages_size += USERVER_NAMESPACE::utils::statistics::Rate{message_size};
    }

    void AccountAlienMessage() { messages_alien_count += USERVER_NAMESPACE::utils::statistics::Rate{1}; }

    PubsubChannelStatistics& operator+=(const PubsubChannelStatistics& other) {
        subscription_timestamp = std::chrono::steady_clock::time_point();
        messages_count += other.messages_count;
        messages_size += other.messages_size;
        messages_alien_count += other.messages_alien_count;
        messages_discarded += other.messages_discarded;
        return *this;
    }
};

struct PubsubShardStatistics {
    std::unordered_map<std::string, PubsubChannelStatistics> by_channel;
    std::string shard_name;

    PubsubChannelStatistics SumByChannel() const {
        PubsubChannelStatistics ret;
        for (const auto& [name, channel_stats] : by_channel) {
            ret += channel_stats;
        }
        return ret;
    }
};

struct RawPubsubClusterStatistics {
    std::vector<PubsubShardStatistics> by_shard;
};

struct PubsubClusterStatistics {
    PubsubClusterStatistics(const PubsubMetricsSettings& settings, bool per_channel_stats_enabled)
        : settings(settings),
          per_channel_stats_enabled(per_channel_stats_enabled)
    {}

    const PubsubMetricsSettings& settings;
    const bool per_channel_stats_enabled;
    std::unordered_map<std::string, PubsubShardStatistics> by_shard;

    PubsubShardStatistics SumByShards() const {
        PubsubShardStatistics sum;
        for (const auto& shard : by_shard) {
            for (const auto& it : shard.second.by_channel) {
                sum.by_channel[it.first] += it.second;
            }
        }
        return sum;
    }

    PubsubChannelStatistics SumByShardsAndChannel() const {
        PubsubChannelStatistics sum;
        for (const auto& shard : by_shard) {
            for (const auto& it : shard.second.by_channel) {
                sum += it.second;
            }
        }
        return sum;
    }
};

void DumpMetric(utils::statistics::Writer& writer, const PubsubChannelStatistics& stats);

void DumpMetric(utils::statistics::Writer& writer, const PubsubShardStatistics& stats);

void DumpMetric(utils::statistics::Writer& writer, const PubsubClusterStatistics& stats);

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
