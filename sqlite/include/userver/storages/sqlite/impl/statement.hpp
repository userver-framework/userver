#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <boost/pfr/core.hpp>

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

  // Prepare statement
  template <typename... Args>
  void UpdateParamsBindings(const Args&... args);
  template <typename T>
  void UpdateRowAsParamsBindings(const T& row);
  void Bind(const int index, const int32_t value) override;
  void Bind(const int index, const uint32_t value) override;
  void Bind(const int index, const int64_t value) override;
  void Bind(const int index, const uint64_t value) override;
  void Bind(const int index, const double value) override;
  void Bind(const int index, const std::string& value) override;
  void Bind(const int index, const std::string_view value) override;
  void Bind(const int index, const char* value, const int size) override;
  void Bind(const int index) override;
  void Reset() noexcept;

  // Execution
  int ColumnCount() const noexcept override;
  int RowsAffected() const noexcept override;
  int LastInsertRowId() const noexcept override;
  bool HasNext() const noexcept override;
  bool IsDone() const noexcept override;
  bool IsFail() const noexcept;
  void Next() override;
  void CheckFail() const;

  // Extract
  void Extract(int column, int8_t& val) const noexcept override;
  void Extract(int column, uint8_t& val) const noexcept override;
  void Extract(int column, int16_t& val) const noexcept override;
  void Extract(int column, uint16_t& val) const noexcept override;
  void Extract(int column, int32_t& val) const noexcept override;
  void Extract(int column, uint32_t& val) const noexcept override;
  void Extract(int column, int64_t& val) const noexcept override;
  void Extract(int column, uint64_t& val) const noexcept override;
  void Extract(int column, float& val) const noexcept override;
  void Extract(int column, double& val) const noexcept override;
  void Extract(int column, std::string& val) const noexcept override;
  void Extract(int column, std::vector<uint8_t>& val) const noexcept override;

 private:
  void CheckCode(const int ret_code) const;

  struct SQLiteStatementDeleter {
    void operator()(sqlite3_stmt* stmt);
  };

  using NativeStatementPtr =
      std::unique_ptr<sqlite3_stmt, SQLiteStatementDeleter>;

  NativeStatementPtr prepareStatement(const std::string& statement_str);

  const NativeHandler& db_handler_;
  NativeStatementPtr prepare_statement_;
  int column_count_;
  int exec_status_ = 0;
};

template <typename... Args>
void Statement::UpdateParamsBindings(const Args&... args) {
  int index = 1;
  (Bind(index++, args), ...);
}

template <typename T>
void Statement::UpdateRowAsParamsBindings(const T& row) {
  static_assert(std::is_aggregate_v<T> || boost::pfr::tuple_size_v<T> > 0,
                "T must be an aggregate type or tuple-like type");
  if constexpr (std::is_aggregate_v<T>) {
    auto fields = boost::pfr::structure_to_tuple(row);
    std::apply(
        [this](const auto&... args) { this->UpdateParamsBindings(args...); },
        fields);
  } else {
    return std::apply(
        [this](const auto&... args) { this->UpdateParamsBindings(args...); },
        row);
  }
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
