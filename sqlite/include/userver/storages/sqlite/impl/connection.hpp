#pragma once

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/zstring_view.hpp>

#include <userver/storages/sqlite/impl/native_handler.hpp>
#include <userver/storages/sqlite/impl/result_wrapper.hpp>
#include <userver/storages/sqlite/impl/statement.hpp>
#include <userver/storages/sqlite/impl/statements_cache.hpp>
#include <userver/storages/sqlite/infra/connection_ptr.hpp>
#include <userver/storages/sqlite/infra/statistics/statistics.hpp>
#include <userver/storages/sqlite/infra/statistics/statistics_counter.hpp>
#include <userver/storages/sqlite/operation_types.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/result_set.hpp>
#include <userver/storages/sqlite/sqlite_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class Connection {
public:
    Connection(
        const settings::SQLiteSettings& settings,
        engine::TaskProcessor& blocking_task_processor,
        infra::statistics::PoolStatistics& stat
    );

    ~Connection();

    const settings::ConnectionSettings& GetSettings() const noexcept;

    StatementPtr PrepareStatement(const Query& query);

    void ExecutionStep(StatementBasePtr prepare_statement) const;

    void Begin(const settings::TransactionOptions& options);
    void Commit();
    void Rollback();

    void Save(const std::string& name);
    void Release(const std::string& name);
    void RollbackTo(const std::string& name);

    void AccountQueryExecute() noexcept;
    void AccountQueryCompleted() noexcept;
    void AccountQueryFailed() noexcept;
    void AccountTransactionStart() noexcept;
    void AccountTransactionCommit() noexcept;
    void AccountTransactionRollback() noexcept;

    // TODO: is used ?
    bool IsBroken() const;
    void NotifyBroken();

private:
    void ExecuteQuery(utils::zstring_view query) const;

    engine::TaskProcessor& blocking_task_processor_;
    impl::NativeHandler db_handler_;
    settings::SQLiteSettings settings_;
    impl::StatementsCache statements_cache_;
    infra::statistics::QueryStatCounter queries_stat_counter_;
    infra::statistics::TransactionStatCounter transactions_stat_counter_;
    std::atomic<bool> broken_{false};
};

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
