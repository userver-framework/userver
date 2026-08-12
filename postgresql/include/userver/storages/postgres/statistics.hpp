#pragma once

/// @file userver/storages/postgres/statistics.hpp
/// @brief Statistics helpers

#include <string>
#include <unordered_map>
#include <vector>

#include <userver/storages/postgres/detail/time_types.hpp>

#include <userver/congestion_control/controllers/linear.hpp>
#include <userver/utils/statistics/min_max_avg.hpp>
#include <userver/utils/statistics/percentile.hpp>
#include <userver/utils/statistics/rate.hpp>
#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/utils/statistics/recentperiod.hpp>
#include <userver/utils/statistics/relaxed_counter.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

/// @brief Template transaction statistics storage
template <typename GaugeCounter, typename RateCounter, typename PercentileAccumulator>
struct TransactionStatistics {
    /// Number of transactions started
    RateCounter total{};
    /// Number of transactions committed
    RateCounter commit_total{};
    /// Number of transactions rolled back
    RateCounter rollback_total{};
    /// Number of out-of-transaction executions
    RateCounter out_of_trx_total{};
    /// Number of parsed queries
    RateCounter parse_total{};
    /// Number of query executions
    RateCounter execute_total{};
    /// Total number of replies
    RateCounter reply_total{};
    /// Number of portal bind operations
    RateCounter portal_bind_total{};
    /// Error during query execution
    RateCounter error_execute_total{};
    /// Timeout while executing query
    RateCounter execute_timeout{};
    /// Duplicate prepared statements
    /// This is not a hard error, the prepared statements are quite reusable due
    /// to pretty uniqueness of names. Nevertheless we would like to see them to
    /// diagnose certain kinds of problems
    RateCounter duplicate_prepared_statements{};

    // TODO pick reasonable resolution for transaction
    // execution times
    /// Transaction overall execution time distribution
    PercentileAccumulator total_percentile;
    /// Transaction aggregated query execution time distribution
    PercentileAccumulator busy_percentile;
    /// Transaction wait for pool time (difference between trx_start_time and
    /// work_start_time)
    PercentileAccumulator wait_start_percentile;
    /// Transaction wait for pool time (difference between last_execute_finish and
    /// trx_end_time)
    PercentileAccumulator wait_end_percentile;
    /// Return to pool percentile (difference between trx_end_time and time the
    /// connection has been returned to the pool)
    PercentileAccumulator return_to_pool_percentile;
};

/// @brief Template connection statistics storage
template <typename GaugeCounter, typename RateCounter, typename MmaAccumulator>
struct ConnectionStatistics {
    /// Number of connections opened
    RateCounter open_total{};
    /// Number of connections dropped
    RateCounter drop_total{};
    /// Number of active connections
    GaugeCounter active = 0;
    /// Number of connections in use
    GaugeCounter used = 0;
    /// Number of maximum allowed connections
    GaugeCounter maximum = 0;
    /// Number of waiting requests
    GaugeCounter waiting = 0;
    /// Error during connection
    RateCounter error_total{};
    /// Connection timeouts (timeouts while connecting)
    RateCounter error_timeout{};
    /// Number of rejected connection attempts due to rate limiting
    RateCounter rate_limit_throttled{};
    /// Number of maximum allowed waiting requests
    GaugeCounter max_queue_size = 0;

    /// Prepared statements count min-max-avg
    MmaAccumulator prepared_statements;
};

/// @brief Template instance topology statistics storage
template <typename MmaAccumulator>
struct InstanceTopologyStatistics {
    /// Roundtrip time min-max-avg
    MmaAccumulator roundtrip_time;
    /// Replication lag min-max-avg
    MmaAccumulator replication_lag;
};

/// @brief Template instance statistics storage
template <typename GaugeCounter, typename RateCounter, typename PercentileAccumulator, typename MmaAccumulator>
struct InstanceStatisticsTemplate {
    /// Connection statistics
    ConnectionStatistics<GaugeCounter, RateCounter, MmaAccumulator> connection;
    /// Transaction statistics
    TransactionStatistics<GaugeCounter, RateCounter, PercentileAccumulator> transaction;
    /// Topology statistics
    InstanceTopologyStatistics<MmaAccumulator> topology;
    /// Error caused by pool exhaustion
    RateCounter pool_exhaust_errors{};
    /// Error caused by queue size overflow
    RateCounter queue_size_errors{};
    /// Connect time percentile
    PercentileAccumulator connection_percentile;
    /// Acquire connection percentile
    PercentileAccumulator acquire_percentile;
    /// Congestion control statistics
    std::conditional_t<std::is_same_v<GaugeCounter, uint32_t>, std::byte /* NOOP */, congestion_control::v2::Stats>
        congestion_control{};
};

using RateCounter = USERVER_NAMESPACE::utils::statistics::RateCounter;
using Rate = USERVER_NAMESPACE::utils::statistics::Rate;
using Percentile = USERVER_NAMESPACE::utils::statistics::Percentile<2048>;
using MinMaxAvg = USERVER_NAMESPACE::utils::statistics::MinMaxAvg<uint32_t>;
using InstanceStatistics = InstanceStatisticsTemplate<
    USERVER_NAMESPACE::utils::statistics::RelaxedCounter<uint32_t>,
    RateCounter,
    USERVER_NAMESPACE::utils::statistics::RecentPeriod<Percentile, Percentile, detail::SteadyCoarseClock>,
    USERVER_NAMESPACE::utils::statistics::RecentPeriod<MinMaxAvg, MinMaxAvg, detail::SteadyCoarseClock>>;

struct StatementStatistics final {
    Percentile timings{};
    RateCounter executed{};
    RateCounter errors{};

    void Add(const StatementStatistics& other) {
        timings.Add(other.timings);
        executed.Add(other.executed.Load());
        errors.Add(other.errors.Load());
    }
};

using InstanceStatisticsNonatomicBase = InstanceStatisticsTemplate<uint32_t, Rate, Percentile, MinMaxAvg>;

struct InstanceStatisticsNonatomic : InstanceStatisticsNonatomicBase {
    InstanceStatisticsNonatomic() = default;

    template <typename Statistics>
    InstanceStatisticsNonatomic(const Statistics& stats) {
        *this = stats;
    }
    InstanceStatisticsNonatomic(InstanceStatisticsNonatomic&&) = default;
    InstanceStatisticsNonatomic& operator=(InstanceStatisticsNonatomic&&) = default;

    InstanceStatisticsNonatomic& Add(
        const InstanceStatistics& stats,
        const decltype(InstanceStatistics::topology)& topology_stats
    ) {
        connection.open_total = stats.connection.open_total.Load();
        connection.drop_total = stats.connection.drop_total.Load();
        connection.active = stats.connection.active;
        connection.used = stats.connection.used;
        connection.maximum = stats.connection.maximum;
        connection.waiting = stats.connection.waiting;
        connection.error_total = stats.connection.error_total.Load();
        connection.error_timeout = stats.connection.error_timeout.Load();
        connection.rate_limit_throttled = stats.connection.rate_limit_throttled.Load();
        connection.prepared_statements = stats.connection.prepared_statements.GetStatsForPeriod();
        connection.max_queue_size = stats.connection.max_queue_size;

        transaction.total = stats.transaction.total.Load();
        transaction.commit_total = stats.transaction.commit_total.Load();
        transaction.rollback_total = stats.transaction.rollback_total.Load();
        transaction.out_of_trx_total = stats.transaction.out_of_trx_total.Load();
        transaction.parse_total = stats.transaction.parse_total.Load();
        transaction.execute_total = stats.transaction.execute_total.Load();
        transaction.reply_total = stats.transaction.reply_total.Load();
        transaction.portal_bind_total = stats.transaction.portal_bind_total.Load();
        transaction.error_execute_total = stats.transaction.error_execute_total.Load();
        transaction.execute_timeout = stats.transaction.execute_timeout.Load();
        transaction.duplicate_prepared_statements = stats.transaction.duplicate_prepared_statements.Load();
        transaction.total_percentile = stats.transaction.total_percentile.GetStatsForPeriod();
        transaction.busy_percentile = stats.transaction.busy_percentile.GetStatsForPeriod();
        transaction.wait_start_percentile = stats.transaction.wait_start_percentile.GetStatsForPeriod();
        transaction.wait_end_percentile = stats.transaction.wait_end_percentile.GetStatsForPeriod();
        transaction.return_to_pool_percentile = stats.transaction.return_to_pool_percentile.GetStatsForPeriod();

        topology.roundtrip_time = topology_stats.roundtrip_time.GetStatsForPeriod();
        topology.replication_lag = topology_stats.replication_lag.GetStatsForPeriod();

        pool_exhaust_errors = stats.pool_exhaust_errors.Load();
        queue_size_errors = stats.queue_size_errors.Load();
        connection_percentile = stats.connection_percentile.GetStatsForPeriod();
        acquire_percentile = stats.acquire_percentile.GetStatsForPeriod();

        return *this;
    }

    InstanceStatisticsNonatomic& Add(const std::unordered_map<std::string, StatementStatistics>& stats) {
        for (const auto& [statement_name, statement_stats] : stats) {
            const auto [it, inserted] = per_statement_stats.try_emplace(statement_name, statement_stats);
            if (!inserted) {
                it->second.Add(statement_stats);
            }
        }

        return *this;
    }

    std::unordered_map<std::string, StatementStatistics> per_statement_stats;
};

/// @brief Instance statistics with description
struct InstanceStatsDescriptor {
    /// host[:port] of an instance
    std::string host_port;
    /// Statistics of an instance
    InstanceStatisticsNonatomic stats;
};

/// @brief Cluster statistics storage
struct ClusterStatistics {
    /// Connlimit mode auto is on
    bool connlimit_mode_auto_on{false};
    /// Master instance statistics
    InstanceStatsDescriptor master;
    /// Sync slave instance statistics
    InstanceStatsDescriptor sync_slave;
    /// Slave instances statistics
    std::vector<InstanceStatsDescriptor> slaves;
    /// Unknown/unreachable instances statistics
    std::vector<InstanceStatsDescriptor> unknown;
};

// InstanceStatisticsNonatomic values support for utils::statistics::Writer
void DumpMetric(USERVER_NAMESPACE::utils::statistics::Writer& writer, const InstanceStatisticsNonatomic& stats);

/// @brief InstanceStatsDescriptor values support for utils::statistics::Writer
void DumpMetric(USERVER_NAMESPACE::utils::statistics::Writer& writer, const InstanceStatsDescriptor& value);

/// @brief ClusterStatistics values support for utils::statistics::Writer
void DumpMetric(USERVER_NAMESPACE::utils::statistics::Writer& writer, const ClusterStatistics& value);

using ClusterStatisticsPtr = std::unique_ptr<ClusterStatistics>;

}  // namespace storages::postgres

USERVER_NAMESPACE_END
