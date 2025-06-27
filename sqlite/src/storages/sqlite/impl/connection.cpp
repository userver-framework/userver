#include <userver/storages/sqlite/impl/connection.hpp>

#include <userver/engine/async.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/string_literal.hpp>

#include <userver/storages/sqlite/infra/statistics/statistics.hpp>
#include <userver/storages/sqlite/infra/statistics/statistics_counter.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/sqlite_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

namespace {
constexpr utils::StringLiteral kStatementTransactionSerializableIsolationLevel = "PRAGMA read_uncommitted=0";
constexpr utils::StringLiteral kStatementTransactionReadUncommittedIsolationLevel = "PRAGMA read_uncommitted=1";
constexpr utils::StringLiteral kStatementTransactionBeginDeferred = "BEGIN DEFERRED";
constexpr utils::StringLiteral kStatementTransactionBeginImmediate = "BEGIN IMMEDIATE";
constexpr utils::StringLiteral kStatementTransactionBeginExclusive = "BEGIN EXCLUSIVE";
constexpr utils::StringLiteral kStatementTransactionCommit = "COMMIT TRANSACTION";
constexpr utils::StringLiteral kStatementTransactionRollback = "ROLLBACK TRANSACTION";
constexpr utils::StringLiteral kStatementSavepointBegin = "SAVEPOINT ";
constexpr utils::StringLiteral kStatementSavepointRelease = "RELEASE SAVEPOINT ";
constexpr utils::StringLiteral kStatementSavepointRollbackTo = "ROLLBACK TO SAVEPOINT ";

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

const settings::ConnectionSettings& Connection::GetSettings() const noexcept { return settings_.conn_settings; }

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
        !settings_.read_uncommitted) {
        ExecuteQuery(kStatementTransactionReadUncommittedIsolationLevel);
    }
    switch (options.mode) {
        case settings::TransactionOptions::kDeferred:
            ExecuteQuery(kStatementTransactionBeginDeferred);
            break;
        case settings::TransactionOptions::kImmediate:
            ExecuteQuery(kStatementTransactionBeginImmediate);
            break;
        case settings::TransactionOptions::kExclusive:
            ExecuteQuery(kStatementTransactionBeginExclusive);
            break;
        default:
            break;
    }
    AccountTransactionStart();
}

void Connection::Commit() {
    ExecuteQuery(kStatementTransactionCommit);
    AccountTransactionCommit();
    if (!settings_.read_uncommitted) {
        ExecuteQuery(kStatementTransactionSerializableIsolationLevel);
    }
}

void Connection::Rollback() {
    ExecuteQuery(kStatementTransactionRollback);
    AccountTransactionRollback();
    if (!settings_.read_uncommitted) {
        ExecuteQuery(kStatementTransactionSerializableIsolationLevel);
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

void Connection::ExecuteQuery(utils::zstring_view query) const { db_handler_.Exec(query); }

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
