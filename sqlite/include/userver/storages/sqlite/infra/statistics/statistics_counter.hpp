#pragma once

#include <userver/storages/sqlite/infra/statistics/statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra::statistics {

// TODO: make RAII alghorithms

class QueryStatCounter {
public:
    explicit QueryStatCounter(PoolQueriesStatistics& stats);

    ~QueryStatCounter();

    QueryStatCounter(const QueryStatCounter&) = delete;
    QueryStatCounter& operator=(const QueryStatCounter&) = delete;

    void AccountQueryExecute() noexcept;
    void AccountQueryCompleted() noexcept;
    void AccountQueryFailed() noexcept;

private:
    PoolQueriesStatistics& queries_stats_;
    utils::datetime::SteadyClock::time_point exec_begin_time;
};

class TransactionStatCounter {
public:
    explicit TransactionStatCounter(PoolTransactionsStatistics& stats);

    ~TransactionStatCounter();

    TransactionStatCounter(const TransactionStatCounter&) = delete;
    TransactionStatCounter& operator=(const TransactionStatCounter&) = delete;

    void AccountTransactionStart() noexcept;
    void AccountTransactionCommit() noexcept;
    void AccountTransactionRollback() noexcept;

private:
    PoolTransactionsStatistics& transations_stats_;
    utils::datetime::SteadyClock::time_point exec_begin_time;
};

}  // namespace storages::sqlite::infra::statistics

USERVER_NAMESPACE_END
