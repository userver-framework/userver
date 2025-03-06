#include <userver/storages/sqlite/impl/connection.hpp>

#include <userver/storages/sqlite/impl/statements.hpp>

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
      settings_{settings.conn_settings},
      db_handler_{OpenDatabase(settings)},
      statements_cache_{db_handler_.get(),
                        settings.conn_settings.max_prepared_cache_size} {}

Connection::~Connection() = default;

settings::ConnectionSettings const& Connection::GetSettings() const noexcept {
  return settings_;
}

sqlite3* Connection::GetHandle() const noexcept { return db_handler_.get(); }

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

void Connection::SQLiteHandlerDeleter::operator()(sqlite3* sqlite_handle) {
  // TODO: is this an I/O bound operation, does it need to be run on
  // blocking_task_processor_?
  sqlite3_close(sqlite_handle);

  // TODO: error is SQLITE_BUSY: "database is locked"
}

sqlite3* Connection::OpenDatabase(const settings::SQLiteSettings& settings) {
  int flags = 0;
  if (settings.read_mode == settings::SQLiteSettings::ReadMode::kReadOnly) {
    flags |= SQLITE_OPEN_READONLY;
  } else {
    flags |= SQLITE_OPEN_READWRITE;
  }
  if (settings.create_file &&
      settings.read_mode == settings::SQLiteSettings::ReadMode::kReadWrite) {
    flags |= SQLITE_OPEN_CREATE;
  }
  if (settings.shared_cashe) {
    flags |= SQLITE_OPEN_SHAREDCACHE;
  }
  sqlite3* handle = nullptr;
  // TODO: is this an I/O bound operation, does it need to be run on
  // blocking_task_processor_?
  // TODO: sqlite3_open_v2 thread-safe?
  if (const int ret =
          sqlite3_open_v2(settings.db_name.c_str(), &handle, flags, nullptr);
      ret != SQLITE_OK) {
    // TODO: Take logic into the NativeInterface class and make full RAII
    sqlite3_close(handle);
    NotifyBroken();
    throw SQLiteException(handle, ret);
  }
  sqlite3_wal_checkpoint(handle, nullptr);
  return handle;
}

std::shared_ptr<Statement> Connection::MakeStatement(
    const std::string& statement) const {
  return std::make_shared<Statement>(db_handler_.get(), statement);
}

bool Connection::IsBroken() const { return broken_.load(); }

void Connection::NotifyBroken() { broken_.store(true); }

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
