#include <storages/odbc/detail/statement_stats_storage.hpp>

#include <algorithm>
#include <chrono>
#include <utility>

#include <userver/engine/sleep.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

namespace {

constexpr std::size_t kEventsQueueSize = 1000;

}  // namespace

StatementStatsStorage::StatementStatsStorage(const settings::StatementMetricsSettings& settings)
    : stats_{std::make_unique<Storage>(std::max<std::size_t>(settings.max_statements, 1))},
      events_queue_{Queue::Create(kEventsQueueSize)},
      ensure_lifetime_producer_{events_queue_->GetProducer()},
      enabled_{settings.max_statements != 0},
      max_statements_{settings.max_statements}
{
    consumer_task_ = utils::Async("odbc-statement-stats-consumer", [this] { ProcessEvents(); });
}

StatementStatsStorage::~StatementStatsStorage() {
    if (consumer_task_.IsValid()) {
        consumer_task_.SyncCancel();
    }
}

void StatementStatsStorage::Account(std::string_view statement_name, std::size_t duration_ms, ExecutionResult result)
    const noexcept {
    const auto generation = GetGenerationToken();
    if (!generation) {
        return;
    }
    Account(statement_name, duration_ms, result, *generation);
}

void StatementStatsStorage::Account(
    std::string_view statement_name,
    std::size_t duration_ms,
    ExecutionResult result,
    GenerationToken generation
) const noexcept {
    if (!IsEnabled()) {
        return;
    }
    try {
        auto producer = events_queue_->GetProducer();
        [[maybe_unused]] const auto pushed =
            producer.PushNoblock(std::make_unique<
                                 StatementEvent>(std::string{statement_name}, duration_ms, result, generation));
    } catch (...) {
        // Metrics must never affect query execution.
    }
}

std::optional<StatementStatsStorage::GenerationToken> StatementStatsStorage::GetGenerationToken() const noexcept {
    const auto generation = generation_.load(std::memory_order_acquire);
    if (!IsEnabled() || generation != generation_.load(std::memory_order_acquire)) {
        return std::nullopt;
    }
    return generation;
}

StatementStatisticsSnapshot StatementStatsStorage::GetStatementsStats() const {
    if (!IsEnabled()) {
        return {};
    }

    const auto locked = stats_->SharedLock();
    StatementStatisticsSnapshot snapshot;
    snapshot.statements.reserve(locked->GetSize());
    locked->VisitAll([&snapshot](const std::string& name, const std::unique_ptr<StoredStatementStats>& stored) {
        snapshot.statements.emplace(name, stored->statistics);
    });
    return snapshot;
}

void StatementStatsStorage::SetSettings(const settings::StatementMetricsSettings& settings) {
    if (max_statements_ == settings.max_statements) {
        return;
    }

    const auto was_enabled = max_statements_ != 0;
    const auto will_be_enabled = settings.max_statements != 0;

    if (!will_be_enabled) {
        enabled_.store(false, std::memory_order_release);
        generation_.fetch_add(1, std::memory_order_acq_rel);
    }

    {
        auto locked = stats_->UniqueLock();
        if (!will_be_enabled || !was_enabled) {
            locked->Clear();
        }
        if (will_be_enabled) {
            locked->SetMaxSize(settings.max_statements);
        }
    }

    max_statements_ = settings.max_statements;
    if (will_be_enabled && !was_enabled) {
        generation_.fetch_add(1, std::memory_order_acq_rel);
        enabled_.store(true, std::memory_order_release);
    }
}

bool StatementStatsStorage::IsEnabled() const noexcept { return enabled_.load(std::memory_order_acquire); }

void StatementStatsStorage::WaitForExhaustion() const {
    auto barrier = std::make_shared<engine::Promise<void>>();
    auto future = barrier->get_future();
    auto event = std::make_unique<StatementEvent>(barrier);
    auto producer = events_queue_->GetProducer();
    while (!producer.PushNoblock(std::move(event))) {
        UINVARIANT(!consumer_task_.IsFinished(), "ODBC statement metrics consumer stopped before drain barrier");
        engine::Yield();
    }
    UINVARIANT(
        future.wait_for(std::chrono::seconds{5}) == engine::FutureStatus::kReady,
        "Timed out waiting for the ODBC statement metrics drain barrier"
    );
    future.get();
}

void StatementStatsStorage::ProcessEvents() noexcept {
    try {
        auto consumer = events_queue_->GetConsumer();
        EventPtr event;
        while (!engine::current_task::IsCancelRequested() && consumer.Pop(event)) {
            if (!event) {
                continue;
            }
            if (event->barrier) {
                try {
                    event->barrier->set_value();
                } catch (...) {
                    // A cancelled test waiter must not stop the consumer.
                }
                continue;
            }
            AccountEvent(*event);
        }
    } catch (...) {
        // The consumer is telemetry-only. Pool teardown cancels this task.
    }
}

void StatementStatsStorage::AccountEvent(const StatementEvent& event) noexcept {
    if (!IsEnabled() || event.generation != generation_.load(std::memory_order_acquire)) {
        return;
    }

    try {
        auto locked = stats_->UniqueLock();
        if (!IsEnabled() || event.generation != generation_.load(std::memory_order_acquire)) {
            return;
        }

        auto* stored = locked->Get(event.statement_name);
        if (!stored) {
            locked->Put(event.statement_name, std::make_unique<StoredStatementStats>());
            stored = locked->Get(event.statement_name);
        }
        if (!stored) {
            return;
        }

        switch (event.result) {
            case ExecutionResult::kSuccess:
                (*stored)->statistics.timings.Account(event.duration_ms);
                ++(*stored)->statistics.executed;
                break;
            case ExecutionResult::kError:
                ++(*stored)->statistics.errors;
                break;
        }
    } catch (...) {
        // Metrics must never affect the consumer or query execution.
    }
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
