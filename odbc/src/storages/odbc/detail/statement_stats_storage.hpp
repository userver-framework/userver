#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <userver/cache/lru_map.hpp>
#include <userver/concurrent/mpsc_queue.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/engine/future.hpp>
#include <userver/engine/shared_mutex.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/storages/odbc/settings.hpp>

#include <storages/odbc/detail/statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

class StatementStatsStorage final {
public:
    using GenerationToken = std::size_t;

    enum class ExecutionResult { kSuccess, kError };

    explicit StatementStatsStorage(const settings::StatementMetricsSettings& settings);
    ~StatementStatsStorage();

    StatementStatsStorage(const StatementStatsStorage&) = delete;
    StatementStatsStorage& operator=(const StatementStatsStorage&) = delete;

    void Account(std::string_view statement_name, std::size_t duration_ms, ExecutionResult result) const noexcept;
    void Account(
        std::string_view statement_name,
        std::size_t duration_ms,
        ExecutionResult result,
        GenerationToken generation
    ) const noexcept;

    std::optional<GenerationToken> GetGenerationToken() const noexcept;

    StatementStatisticsSnapshot GetStatementsStats() const;
    void SetSettings(const settings::StatementMetricsSettings& settings);
    bool IsEnabled() const noexcept;

    /// Wait until all events queued before this call are processed.
    /// Intended for deterministic tests only.
    void WaitForExhaustion() const;

private:
    struct StatementEvent final {
        StatementEvent(
            std::string statement_name,
            std::size_t duration_ms,
            ExecutionResult result,
            GenerationToken generation
        )
            : statement_name{std::move(statement_name)},
              duration_ms{duration_ms},
              result{result},
              generation{generation}
        {}

        explicit StatementEvent(std::shared_ptr<engine::Promise<void>> barrier)
            : barrier{std::move(barrier)}
        {}

        std::string statement_name;
        std::size_t duration_ms{0};
        ExecutionResult result{ExecutionResult::kError};
        GenerationToken generation{0};
        std::shared_ptr<engine::Promise<void>> barrier;
    };

    using EventPtr = std::unique_ptr<StatementEvent>;

    struct StoredStatementStats final {
        StatementStatistics statistics{};
    };

    using Queue = concurrent::MpscQueue<EventPtr>;
    using StorageType = cache::LruMap<std::string, std::unique_ptr<StoredStatementStats>>;
    using Storage = concurrent::Variable<StorageType, engine::SharedMutex>;

    void ProcessEvents() noexcept;
    void AccountEvent(const StatementEvent& event) noexcept;

    std::unique_ptr<Storage> stats_;
    std::shared_ptr<Queue> events_queue_;
    Queue::Producer ensure_lifetime_producer_;
    engine::TaskWithResult<void> consumer_task_;

    std::atomic<bool> enabled_{false};
    std::atomic<std::size_t> generation_{0};
    std::size_t max_statements_{0};
};

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
