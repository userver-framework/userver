#pragma once

#include <storages/redis/impl/redis_stats.hpp>
#include <userver/concurrent/variable.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

class ClusterTopology;

class StatisticsHolder {
public:
    using Stats = Statistics;

    StatisticsHolder()
        : storage_(std::make_unique<Stats>())
    {}

    Stats& MakeInstanceStats() const { return *storage_; }

    void GetStatistics(SentinelStatistics& stats, const ClusterTopology& topology) const;

private:
    std::unique_ptr<Stats> storage_{};
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
