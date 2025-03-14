#include <userver/storages/sqlite/impl/connection.hpp>
#include "userver/storages/sqlite/sqlite_fwd.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

namespace {

constexpr std::string_view kStatementTransactionBeginDeferred =
    "BEGIN DEFERRED";
constexpr std::string_view kStatementTransactionBeginImmediate =
    "BEGIN IMMEDIATE";
constexpr std::string_view kStatementTransactionBeginExclusive =
    "BEGIN EXCLUSIVE";
constexpr std::string_view kStatementTransactionCommit = "COMMIT TRANSACTION";
constexpr std::string_view kStatementTransactionRollback =
    "ROLLBACK TRANSACTION";
constexpr std::string_view kStatementSavepointBegin = "SAVEPOINT ";
constexpr std::string_view kStatementSavepointRelease = "RELEASE SAVEPOINT ";
constexpr std::string_view kStatementSavepointRollbackTo =
    "ROLLBACK TO SAVEPOINT ";
constexpr std::string_view kStatementPrepeareString = "SELECT quote(?)";

}  // namespace

Connection::Connection(const settings::SQLiteSettings& settings,
                       engine::TaskProcessor& blocking_task_processor)
    : blocking_task_processor_{blocking_task_processor},
      db_handler_{settings},
      connection_settings_{settings.conn_settings},
      statements_cache_{db_handler_,
                        connection_settings_.max_prepared_cache_size} {}

Connection::~Connection() = default;

settings::ConnectionSettings const& Connection::GetSettings() const noexcept {
  return connection_settings_;
}

ResultSet Connection::ExecuteCommand(
    settings::OptionalCommandControl optional_cc [[maybe_unused]],
    impl::StatementBasePtr prepare_statement) const {
  return engine::AsyncNoSpan(
             blocking_task_processor_,
             [&prepare_statement] {
               prepare_statement->Next();
               prepare_statement->CheckFail();
               return ResultSet{
                   std::make_unique<impl::ResultWrapper>(prepare_statement)};
             })
      .Get();
}

void Connection::Begin(const settings::TransactionOptions& options) {
  switch (options.mode) {
    case settings::TransactionOptions::kDeferred:
      ExecuteCommandNoPrepare(kStatementTransactionBeginDeferred.data());
      break;
    case settings::TransactionOptions::kImmediate:
      ExecuteCommandNoPrepare(kStatementTransactionBeginImmediate.data());
      break;
    case settings::TransactionOptions::kExclusive:
      ExecuteCommandNoPrepare(kStatementTransactionBeginExclusive.data());
      break;
    default:
      break;
  }
}

void Connection::Commit() {
  ExecuteCommandNoPrepare(kStatementTransactionCommit.data());
}

void Connection::Rollback() {
  ExecuteCommandNoPrepare(kStatementTransactionRollback.data());
}

void Connection::Savepoint(const std::string& name) {
  ExecuteCommandNoPrepare(std::string(kStatementSavepointBegin) + name);
}

void Connection::Release(const std::string& name) {
  ExecuteCommandNoPrepare(std::string(kStatementSavepointRelease) + name);
}

void Connection::RollbackTo(const std::string& name) {
  ExecuteCommandNoPrepare(std::string(kStatementSavepointRollbackTo) + name);
}

std::string Connection::PrepareString(const std::string& str) {
  return ExecuteCommandNoPrepare(kStatementPrepeareString.data(), str)
      .AsSingleField<std::string>();
}

StatementPtr Connection::PrepareStatement(const Query& query) {
  if (connection_settings_.prepared_statements ==
      settings::ConnectionSettings::kNoPreparedStatements) {
    return std::make_shared<Statement>(db_handler_, query.GetStatement());
  } else {
    auto stmt = statements_cache_.PrepareStatement(query.GetStatement());
    stmt->Reset();
    return stmt;
  }
}

bool Connection::IsBroken() const { return broken_.load(); }

void Connection::NotifyBroken() { broken_.store(true); }

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
