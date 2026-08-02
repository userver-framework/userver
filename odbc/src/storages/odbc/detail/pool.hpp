#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <storages/odbc/detail/conn_ptr.hpp>
#include <storages/odbc/detail/connection.hpp>
#include <storages/odbc/detail/prepared_statement_cache.hpp>
#include <storages/odbc/detail/statement_stats_storage.hpp>
#include <storages/odbc/detail/statistics.hpp>
#include <string>
#include <userver/drivers/impl/connection_pool_base.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/periodic_task.hpp>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

class Pool final : public drivers::impl::ConnectionPoolBase<Connection, Pool> {
public:
    Pool(
        const std::string& dsn,
        std::size_t min_pool_size,
        std::size_t max_pool_size,
        engine::TaskProcessor& blocking_task_processor,
        const settings::StatementMetricsSettings& statement_metrics_settings = {},
        const settings::PreparedStatementCacheSettings& prepared_statement_cache_settings = {}
    );
    Pool(
        std::vector<std::string> dsns,
        std::size_t min_pool_size,
        std::size_t max_pool_size,
        engine::TaskProcessor& blocking_task_processor,
        const settings::StatementMetricsSettings& statement_metrics_settings = {},
        const settings::PreparedStatementCacheSettings& prepared_statement_cache_settings = {}
    );

    ~Pool();

    ConnectionPtr Acquire(engine::Deadline deadline);

    void Release(ConnectionUniquePtr connection);

    const InstanceStatistics& GetStatistics() const noexcept { return stats_; }
    StatementStatisticsSnapshot GetStatementStatistics() const;
    StatementStatsStorage& GetStatementStatsStorage() noexcept { return statement_stats_; }
    void SetStatementMetricsSettings(const settings::StatementMetricsSettings& settings);
    const PreparedStatementCacheStatistics& GetPreparedStatementCacheStatistics() const noexcept;
    void SetPreparedStatementCacheSettings(const settings::PreparedStatementCacheSettings& settings);

    void AccountQueryExecuted(std::chrono::microseconds duration) noexcept;
    void AccountQueryError() noexcept;
    void AccountQueryTimeout() noexcept;
    void AccountOutOfTransaction() noexcept;

    void AccountTransactionStarted() noexcept;
    void AccountTransactionCommit(std::chrono::microseconds total_duration, std::chrono::microseconds busy_duration)
        noexcept;
    void AccountTransactionRollback() noexcept;

private:
    friend class drivers::impl::ConnectionPoolBase<Connection, Pool>;

    ConnectionUniquePtr DoCreateConnection(engine::Deadline deadline);
    void RunSizeMonitor();

    void AccountConnectionCreated() noexcept;
    void AccountConnectionAcquired() noexcept;
    void AccountConnectionReleased() noexcept;
    void AccountConnectionDestroyed() noexcept;
    void AccountOverload() noexcept;

    const std::vector<std::string> dsns_;
    const std::size_t min_pool_size_;
    const std::size_t max_pool_size_;
    engine::TaskProcessor& blocking_task_processor_;
    mutable std::atomic<std::size_t> dsn_index_{0};
    utils::PeriodicTask size_monitor_;

    InstanceStatistics stats_{};
    StatementStatsStorage statement_stats_;
    std::shared_ptr<PreparedStatementCacheState> prepared_statement_cache_state_;
};

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
