#include <userver/storages/sqlite/connection.hpp>

#include <optional>

#include <userver/engine/async.hpp>

#include <userver/storages/sqlite/exceptions.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/result_set.hpp>

#include <sqlite3.h>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

Connection::Connection(const SQLiteSettings& settings [[maybe_unused]],
                       engine::TaskProcessor& blocking_task_processor)
    : blocking_task_processor_(blocking_task_processor) {
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
  db.reset(handle);
}

Connection::~Connection() = default;

void Connection::Deleter::operator()(sqlite3* sqlite_handle) {
  // TODO: is this an I/O bound operation, does it need to be run on
  // blocking_task_processor_?
  sqlite3_close(sqlite_handle);

  // TODO: error is SQLITE_BUSY: "database is locked"
}

sqlite3* Connection::getHandle() const noexcept { return db.get(); };

Transaction Connection::Begin(std::string name,
                              const TransactionOptions& options) const {
  return Begin(std::nullopt, name, options);
}

Transaction Connection::Begin(OptionalCommandControl command_control
                              [[maybe_unused]],
                              std::string name [[maybe_unused]],
                              const TransactionOptions& options
                              [[maybe_unused]]) const {
  return Transaction{getHandle(), blocking_task_processor_};
}

ResultSet Connection::DoExecute(OptionalCommandControl command_controlWWWW
                                [[maybe_unused]],
                                const Query& query [[maybe_unused]],
                                std::optional<std::size_t> batch_size
                                [[maybe_unused]]) const {
  // Prepare statement and execute first step
  // TODO: For simple INSERT, DELETE, UPDATE this works, but for example using
  // RETURNING clauses, obviously repeated calls to sqlite3_step are required to
  // get all rows https://www.sqlite.org/lang_returning.html
  // Based on circumstantial evidence, nested and complex DML queries execute in
  // one sqlite3_step, but this requires inspection and profiling
  return engine::AsyncNoSpan(
             blocking_task_processor_,
             [this, query] {
               sqlite3_stmt* stmt = nullptr;
               int ret = 0;
               // TODO: is this an CPU bound operation, does it need to be run
               // on
               // main_task_processor_?
               if (ret = sqlite3_prepare_v2(getHandle(),
                                            query.GetStatement().c_str(), -1,
                                            &stmt, nullptr);
                   ret != SQLITE_OK) {
                 throw SQLiteException(getHandle(), ret);
               }
               //
               const int exec_status =
                   sqlite3_step(stmt);  // TODO: is this an first-call I/O bound
                                        // operation, does it need to be run on
                                        // blocking_task_processor_?
               if (exec_status != SQLITE_ROW && exec_status != SQLITE_DONE) {
                 throw SQLiteException(getHandle(), exec_status);
               }
               return ResultSet(stmt, exec_status);
             })
      .Get();
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
