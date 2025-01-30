#include <userver/storages/sqlite/connection.hpp>

#include <optional>

#include <sqlite3.h>

#include <userver/storages/sqlite/exceptions.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

Connection::Connection(const SQLiteSettings& settings,
                       engine::TaskProcessor& blocking_task_processor)
    : blocking_task_processor_{blocking_task_processor},
      db_handler_{OpenDatabase(settings)},
      statements_cache_{db_handler_.get(),
                        settings.conn_settings.max_prepared_cache_size} {}
Connection::~Connection() = default;

void Connection::SQLiteHandlerDeleter::operator()(sqlite3* sqlite_handle) {
  // TODO: is this an I/O bound operation, does it need to be run on
  // blocking_task_processor_?
  sqlite3_close(sqlite_handle);

  // TODO: error is SQLITE_BUSY: "database is locked"
}

sqlite3* Connection::OpenDatabase(const SQLiteSettings& settings) const {
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
  if (const int ret =
          sqlite3_open_v2(settings.db_name.c_str(), &handle, flags, nullptr);
      ret != SQLITE_OK) {
    throw SQLiteException(getHandle(), ret);
  }
  return handle;
}

sqlite3* Connection::getHandle() const noexcept { return db_handler_.get(); };

Transaction Connection::Begin(std::string name,
                              const TransactionOptions& options) {
  return Begin(std::nullopt, name, options);
}

Transaction Connection::Begin(OptionalCommandControl command_control
                              [[maybe_unused]],
                              std::string name [[maybe_unused]],
                              const TransactionOptions& options
                              [[maybe_unused]]) {
  return Transaction{getHandle(), blocking_task_processor_, options,
                     statements_cache_};
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
