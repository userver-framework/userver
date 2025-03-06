#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <sqlite3.h>

#include <userver/storages/sqlite/impl/result_wrapper.hpp>
#include <userver/storages/sqlite/impl/statements_base.hpp>
#include <userver/storages/sqlite/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class Statement final : public StatementBase {
 public:
  Statement(sqlite3* db_handler, const std::string& statement);
  ~Statement() override;

  Statement(const Statement& other) = delete;
  Statement(Statement&& other) noexcept;

  void Bind(const int index, const int32_t value) override;
  void Bind(const int index, const int64_t value) override;
  void Bind(const int index, const uint32_t value) override;
  void Bind(const int index, const uint64_t value) override;
  void Bind(const int index, const double value) override;
  void Bind(const int index, const std::string& value) override;
  void Bind(const int index, const std::string_view value) override;
  void Bind(const int index, const char* value, const int size) override;
  void Bind(const int index) override;
  void Reset() noexcept;

  const std::string& GetStatementText() const noexcept;
  std::string getExpandedStatementText() const noexcept;

  int RowsAffected() const noexcept override;
  int LastInsertRowId() const noexcept override;
  bool HasNext() const noexcept override;
  bool IsDone() const noexcept override;
  bool IsFail() const noexcept;
  void Next() noexcept override;
  int ColumnCount() const noexcept override;
  void CheckFail() const;

  int32_t GetInt32Column(int column) const noexcept override;
  uint32_t GetUInt32Column(int column) const noexcept override;
  int64_t GetInt64Column(int column) const noexcept override;
  double GetDoubleColumn(int column) const noexcept override;
  const char* GetCStringColumn(int column) const noexcept override;
  std::string GetStringColumn(int column) const noexcept override;
  const void* GetBlobColumn(int column) const noexcept override;
  std::vector<uint8_t> GetBytesColumn(int column) const noexcept override;

 private:
  struct SQLiteStatementDeleter {
    void operator()(sqlite3_stmt* stmt);
  };
  using NativeStatementPtr = std::shared_ptr<sqlite3_stmt>;

  NativeStatementPtr prepareStatement();

  sqlite3* db_handler_;    // Pointer to SQLite Database Connection Handle
  std::string statement_;  // UTF-8 SQL Query
  NativeStatementPtr prepare_statement_;  //  Shared Pointer to the prepared
                                          //  SQLite Statement Object
  size_t column_count_;  // Number of columns in the result of the prepared
  // statement
  int exec_status_ = 0;  // TODO: We can get it from sqlite3_errcode, but it
  // maybe unsafe in a multi-thread environment or with
  // invested queries
};

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
