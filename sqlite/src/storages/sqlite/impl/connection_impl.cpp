#include <userver/storages/sqlite/impl/connection_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

ConnectionImpl::ConnectionImpl(const SQLiteSettings& settings,
                               engine::TaskProcessor& blocking_task_processor)
    : blocking_task_processor_{blocking_task_processor},
      settings_{settings.conn_settings},
      db_handler_{OpenDatabase(settings)},
      statements_cache_{db_handler_.get(),
                        settings.conn_settings.max_prepared_cache_size} {}

ConnectionSettings const& ConnectionImpl::GetSettings() const noexcept {
  return settings_;
}

sqlite3* ConnectionImpl::GetHandle() const noexcept {
  return db_handler_.get();
}

void ConnectionImpl::Commit() {
  // TODO: Process the case of a call outside the transaction
  // TODO: How to check already commited?
  //   if (transaction_commited_) {
  //     throw SQLiteException("Transaction already commited");
  //   }
  ExecuteCommandNoPrepare("COMMIT TRANSACTION");
}

void ConnectionImpl::Rollback() {
  // TODO: Process the case of a call outside the transaction
  // TODO: How to check already commited?
  //   if (transaction_commited_) {
  //     throw SQLiteException("Transaction already commited");
  //   }
  ExecuteCommandNoPrepare("ROLLBACK TRANSACTION");
}

void ConnectionImpl::SQLiteHandlerDeleter::operator()(sqlite3* sqlite_handle) {
  // TODO: is this an I/O bound operation, does it need to be run on
  // blocking_task_processor_?
  sqlite3_close(sqlite_handle);

  // TODO: error is SQLITE_BUSY: "database is locked"
}

sqlite3* ConnectionImpl::OpenDatabase(const SQLiteSettings& settings) const {
  int flags = 0;
  if (settings.read_mode == SQLiteSettings::ReadMode::kReadOnly) {
    flags |= SQLITE_OPEN_READONLY;
  } else {
    flags |= SQLITE_OPEN_READWRITE;
  }
  if (settings.create_file) {
    flags |= SQLITE_OPEN_CREATE;
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
    throw SQLiteException(handle, ret);
  }
  return handle;
}

std::shared_ptr<Statement> ConnectionImpl::MakeStatement(
    const std::string& statement) const {
  return std::make_shared<Statement>(db_handler_.get(), statement);
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
