#include <iostream>
#include <userver/storages/sqlite/connection.hpp>

#include <optional>

#include "userver/storages/sqlite/exceptions.hpp"
#include "userver/storages/sqlite/options.hpp"
#include "userver/storages/sqlite/result_set.hpp"

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
  if (sqlite3_open_v2(settings.db_name.c_str(), &db, flags, nullptr) !=
      SQLITE_OK) {
    throw USERVER_NAMESPACE::storages::sqlite::SQLiteException(
        0, "Failed to open database: " + std::string(sqlite3_errmsg(db)));
  }
}

Connection::~Connection() {
  if (db) {
    sqlite3_close(db);
  }
};

Transaction Connection::Begin(std::string name,
                              const TransactionOptions& options) const {
  return Begin(std::nullopt, name, options);
}

Transaction Connection::Begin(OptionalCommandControl command_control
                              [[maybe_unused]],
                              std::string name [[maybe_unused]],
                              const TransactionOptions& options
                              [[maybe_unused]]) const {
  return Transaction{};
}

ResultSet Connection::DoExecute(OptionalCommandControl command_controlWWWW
                                [[maybe_unused]],
                                const Query& query [[maybe_unused]],
                                std::optional<std::size_t> batch_size
                                [[maybe_unused]]) const {
  // TODO:
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, query.GetStatement().c_str(), -1, &stmt,
                         nullptr) != SQLITE_OK) {
    throw std::runtime_error("Failed to prepare statement: " +
                             std::string(sqlite3_errmsg(db)));
  }
  ResultSet result_set;
  int step_result = 0;
  while ((step_result = sqlite3_step(stmt)) == SQLITE_ROW) {
    int column_count = sqlite3_column_count(stmt);
    for (int col = 0; col < column_count; ++col) {
    }
  }

  if (step_result != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw std::runtime_error("Error during query execution: " +
                             std::string(sqlite3_errmsg(db)));
  }
  sqlite3_finalize(stmt);

  return result_set;
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
