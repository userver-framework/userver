#include <userver/storages/sqlite/infra/statistics/statistics_counter.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra::statistics {

QueryStatCounter::QueryStatCounter(PoolQueriesStatistics& stats) : queries_stats_{stats} {}

QueryStatCounter::~QueryStatCounter() = default;

void QueryStatCounter::AccountQueryExecute() noexcept {
    ++queries_stats_.total;
    exec_begin_time = utils::datetime::SteadyClock::now();
}

void QueryStatCounter::AccountQueryCompleted() noexcept {
    auto now = utils::datetime::SteadyClock::now();
    ++queries_stats_.executed;
    queries_stats_.timings.GetCurrentCounter().Account(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - exec_begin_time).count()
    );
}

void QueryStatCounter::AccountQueryFailed() noexcept { ++queries_stats_.error; }

TransactionStatCounter::TransactionStatCounter(PoolTransactionsStatistics& stats) : transations_stats_{stats} {}

TransactionStatCounter::~TransactionStatCounter() = default;

void TransactionStatCounter::AccountTransactionStart() noexcept {
    ++transations_stats_.total;
    exec_begin_time = utils::datetime::SteadyClock::now();
}

void TransactionStatCounter::AccountTransactionCommit() noexcept {
    auto now = utils::datetime::SteadyClock::now();
    ++transations_stats_.commit;
    transations_stats_.timings.GetCurrentCounter().Account(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - exec_begin_time).count()
    );
}

void TransactionStatCounter::AccountTransactionRollback() noexcept { ++transations_stats_.rollback; }

}  // namespace storages::sqlite::infra::statistics

USERVER_NAMESPACE_END
