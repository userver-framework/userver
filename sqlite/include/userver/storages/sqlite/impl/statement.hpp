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

    // Info
    OperationType GetOperationType() const noexcept override;

    // Prepare statement
    void Bind(const int index, const std::int32_t value) override;
    void Bind(const int index, const std::uint32_t value) override;
    void Bind(const int index, const std::int64_t value) override;
    void Bind(const int index, const std::uint64_t value) override;
    void Bind(const int index, const double value) override;
    void Bind(const int index, const std::string& value) override;
    void Bind(const int index, const std::string_view value) override;
    void Bind(const int index, const char* value, const int size) override;
    void Bind(const int index, const std::vector<std::uint8_t>& value) override;
    void Bind(const int index) override;
    void Reset() noexcept;

    // Execution
    int ColumnCount() const noexcept override;
    bool HasNext() const noexcept override;
    bool IsDone() const noexcept override;
    bool IsFail() const noexcept override;
    void Next() noexcept override;
    void CheckStepStatus() override;

    // Extract
    std::int64_t RowsAffected() const noexcept override;
    std::int64_t LastInsertRowId() const noexcept override;
    bool IsNull(int column) const noexcept override;
    void Extract(int column, std::int8_t& val) const noexcept override;
    void Extract(int column, std::uint8_t& val) const noexcept override;
    void Extract(int column, std::int16_t& val) const noexcept override;
    void Extract(int column, std::uint16_t& val) const noexcept override;
    void Extract(int column, std::int32_t& val) const noexcept override;
    void Extract(int column, std::uint32_t& val) const noexcept override;
    void Extract(int column, std::int64_t& val) const noexcept override;
    void Extract(int column, std::uint64_t& val) const noexcept override;
    void Extract(int column, float& val) const noexcept override;
    void Extract(int column, double& val) const noexcept override;
    void Extract(int column, std::string& val) const noexcept override;
    void Extract(int column, std::vector<uint8_t>& val) const noexcept override;

private:
    void CheckCode(const int ret_code) const;

    struct SQLiteStatementDeleter {
        void operator()(sqlite3_stmt* stmt);
    };

    using NativeStatementPtr = std::unique_ptr<sqlite3_stmt, SQLiteStatementDeleter>;

    NativeStatementPtr prepareStatement(const std::string& statement_str);

    const NativeHandler& db_handler_;
    NativeStatementPtr prepare_statement_;
    int column_count_;
    int exec_status_ = 0;
};

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
