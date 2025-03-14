#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <userver/storages/sqlite/impl/native_handler.hpp>
#include <userver/storages/sqlite/impl/statement_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class Statement final : public StatementBase {
 public:
  Statement(const NativeHandler& db_handler, const std::string& statement);
  ~Statement() override;

  Statement(const Statement& other) = delete;
  Statement(Statement&& other) noexcept;

  std::string GetStatementText() const noexcept;
  std::string getExpandedStatementText() const noexcept;

  // Prepare statement logic
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

  // Execution and extract result logic
  int RowsAffected() const noexcept override;
  int LastInsertRowId() const noexcept override;
  bool HasNext() const noexcept override;
  bool IsDone() const noexcept override;
  bool IsFail() const noexcept;
  void Next() noexcept override;
  int ColumnCount() const noexcept override;
  void CheckFail() const override;

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

  NativeStatementPtr prepareStatement(const std::string& statement_str);

  const NativeHandler& db_handler_;
  NativeStatementPtr prepare_statement_;
  size_t column_count_;
  int exec_status_ = 0;
};

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
