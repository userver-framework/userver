#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <sqlite3.h>

#include <userver/storages/sqlite/impl/result_wrapper.hpp>
#include <userver/storages/sqlite/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class Statement final {
 public:
  Statement(sqlite3* db_handler, const std::string& statement);
  ~Statement();

  Statement(const Statement& other) = delete;
  Statement(Statement&& other) noexcept;

  const std::string& GetStatementText() const noexcept;

  std::string getExpandedStatementText() const noexcept;

  template <typename... Args>
  ResultSet Execute(const Args&... args);

 private:
  struct SQLiteStatementDeleter {
    void operator()(sqlite3_stmt* stmt);
  };

  using NativeStatementPtr = std::shared_ptr<sqlite3_stmt>;

  void Reset() noexcept;

  void Bind(const int index, const int32_t value);

  void Bind(const int index, const uint32_t value);

  void Bind(const int index, const int64_t value);

  void Bind(const int index, const double value);

  void Bind(const int index, const std::string& value);

  void Bind(const int index, const std::string_view value);

  void Bind(const int index, const char* value, const int size);

  void Bind(const int index);

  template <typename... Args>
  void UpdateParamsBindings(const Args&... args) {
    int index = 1;
    (Bind(index++, args), ...);
  }

  NativeStatementPtr prepareStatement();

  sqlite3* db_handler_;    // Pointer to SQLite Database Connection Handle
  std::string statement_;  // UTF-8 SQL Query
  NativeStatementPtr prepare_statement_;  //  Shared Pointer to the prepared
                                          //  SQLite Statement Object
  size_t column_count_;  // Number of columns in the result of the prepared
                         // statement
};

template <typename... Args>
ResultSet Statement::Execute(const Args&... args) {
  Reset();
  UpdateParamsBindings(args...);

  const int exec_status = sqlite3_step(prepare_statement_.get());
  // TODO: is this an first-call I/O bound
  // operation, does it need to be run on blocking_task_processor_?
  if (exec_status != SQLITE_ROW && exec_status != SQLITE_DONE) {
    throw SQLiteException(db_handler_, exec_status);
  }
  return ResultSet(
      std::make_shared<impl::ResultWrapper>(prepare_statement_, exec_status));
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
