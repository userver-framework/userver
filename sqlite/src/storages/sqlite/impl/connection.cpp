#include <userver/storages/sqlite/impl/connection.hpp>

#include <userver/engine/async.hpp>
#include <userver/logging/log.hpp>

#include <userver/storages/sqlite/infra/statistics/statistics.hpp>
#include <userver/storages/sqlite/infra/statistics/statistics_counter.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/sqlite_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

namespace {
constexpr std::string_view kStatementTransactionSerializableIsolationLevel = "PRAGMA read_uncommitted=0";
constexpr std::string_view kStatementTransactionReadUncommitedIsolationLevel = "PRAGMA read_uncommitted=1";
constexpr std::string_view kStatementTransactionBeginDeferred = "BEGIN DEFERRED";
constexpr std::string_view kStatementTransactionBeginImmediate = "BEGIN IMMEDIATE";
constexpr std::string_view kStatementTransactionBeginExclusive = "BEGIN EXCLUSIVE";
constexpr std::string_view kStatementTransactionCommit = "COMMIT TRANSACTION";
constexpr std::string_view kStatementTransactionRollback = "ROLLBACK TRANSACTION";
constexpr std::string_view kStatementSavepointBegin = "SAVEPOINT ";
constexpr std::string_view kStatementSavepointRelease = "RELEASE SAVEPOINT ";
constexpr std::string_view kStatementSavepointRollbackTo = "ROLLBACK TO SAVEPOINT ";

}  // namespace

Connection::Connection(
    const settings::SQLiteSettings& settings,
    engine::TaskProcessor& blocking_task_processor,
    infra::statistics::PoolStatistics& stat
)
    : blocking_task_processor_{blocking_task_processor},
      db_handler_{settings, blocking_task_processor_},
      settings_{settings},
      statements_cache_{db_handler_, settings.conn_settings.max_prepared_cache_size},
      queries_stat_counter_{stat.queries},
      transactions_stat_counter_{stat.transactions} {
    LOG_INFO() << "SQLite connection initialized.";
}

Connection::~Connection() = default;

settings::ConnectionSettings const& Connection::GetSettings() const noexcept { return settings_.conn_settings; }

StatementPtr Connection::PrepareStatement(const Query& query) {
    if (settings_.conn_settings.prepared_statements == settings::ConnectionSettings::kNoPreparedStatements) {
        return std::make_shared<Statement>(db_handler_, query.GetStatement());
    } else {
        return statements_cache_.PrepareStatement(query.GetStatement());
    }
}

void Connection::ExecutionStep(StatementBasePtr prepare_statement) const {
    engine::AsyncNoSpan(blocking_task_processor_, [prepare_statement] { prepare_statement->Next(); }).Get();
    prepare_statement->CheckStepStatus();
}

void Connection::Begin(const settings::TransactionOptions& options) {
    if (options.isolation_level == settings::TransactionOptions::IsolationLevel::kReadUncommitted &&
        !settings_.read_uncommited) {
        ExecuteQuery(kStatementTransactionReadUncommitedIsolationLevel.data());
    }
    switch (options.mode) {
        case settings::TransactionOptions::kDeferred:
            ExecuteQuery(kStatementTransactionBeginDeferred.data());
            break;
        case settings::TransactionOptions::kImmediate:
            ExecuteQuery(kStatementTransactionBeginImmediate.data());
            break;
        case settings::TransactionOptions::kExclusive:
            ExecuteQuery(kStatementTransactionBeginExclusive.data());
            break;
        default:
            break;
    }
    AccountTransactionStart();
}

void Connection::Commit() {
    ExecuteQuery(kStatementTransactionCommit.data());
    AccountTransactionCommit();
    if (!settings_.read_uncommited) {
        ExecuteQuery(kStatementTransactionSerializableIsolationLevel.data());
    }
}

void Connection::Rollback() {
    ExecuteQuery(kStatementTransactionRollback.data());
    AccountTransactionRollback();
    if (!settings_.read_uncommited) {
        ExecuteQuery(kStatementTransactionSerializableIsolationLevel.data());
    }
}

void Connection::Save(const std::string& name) { ExecuteQuery(std::string(kStatementSavepointBegin) + name); }

void Connection::Release(const std::string& name) { ExecuteQuery(std::string(kStatementSavepointRelease) + name); }

void Connection::RollbackTo(const std::string& name) {
    ExecuteQuery(std::string(kStatementSavepointRollbackTo) + name);
}

void Connection::AccountQueryExecute() noexcept { queries_stat_counter_.AccountQueryExecute(); }

void Connection::AccountQueryCompleted() noexcept { queries_stat_counter_.AccountQueryCompleted(); }

void Connection::AccountQueryFailed() noexcept { queries_stat_counter_.AccountQueryFailed(); }

void Connection::AccountTransactionStart() noexcept { transactions_stat_counter_.AccountTransactionStart(); }

void Connection::AccountTransactionCommit() noexcept { transactions_stat_counter_.AccountTransactionCommit(); }

void Connection::AccountTransactionRollback() noexcept { transactions_stat_counter_.AccountTransactionRollback(); }

bool Connection::IsBroken() const { return broken_.load(); }

void Connection::NotifyBroken() { broken_.store(true); }

void Connection::ExecuteQuery(const std::string& query) const { db_handler_.Exec(query); }

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
