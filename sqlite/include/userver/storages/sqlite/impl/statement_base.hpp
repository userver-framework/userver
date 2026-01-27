#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <userver/storages/sqlite/operation_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class StatementBase {
public:
    virtual ~StatementBase() = default;

    // Info
    virtual OperationType GetOperationType() const noexcept = 0;

    // Bind methods
    virtual void Bind(const int index, const std::int32_t value) = 0;
    virtual void Bind(const int index, const std::int64_t value) = 0;
    virtual void Bind(const int index, const std::uint32_t value) = 0;
    virtual void Bind(const int index, const std::uint64_t value) = 0;
    virtual void Bind(const int index, const double value) = 0;
    virtual void Bind(const int index, const std::string& value) = 0;
    virtual void Bind(const int index, const std::string_view value) = 0;
    virtual void Bind(const int index, const char* value, const int size) = 0;
    virtual void Bind(const int index, const std::vector<std::uint8_t>& value) = 0;
    virtual void Bind(const int index) = 0;

    // Execution methods
    virtual int ColumnCount() const noexcept = 0;
    virtual bool HasNext() const noexcept = 0;
    virtual bool IsDone() const noexcept = 0;
    virtual bool IsFail() const noexcept = 0;
    virtual void Next() noexcept = 0;
    // Can throw exception if step finish with error code
    virtual void CheckStepStatus() = 0;

    // Extract result methods
    virtual std::int64_t RowsAffected() const noexcept = 0;
    virtual std::int64_t LastInsertRowId() const noexcept = 0;
    virtual bool IsNull(int column) const noexcept = 0;
    virtual void Extract(int column, std::int8_t& val) const noexcept = 0;
    virtual void Extract(int column, std::uint8_t& val) const noexcept = 0;
    virtual void Extract(int column, std::int16_t& val) const noexcept = 0;
    virtual void Extract(int column, std::uint16_t& val) const noexcept = 0;
    virtual void Extract(int column, std::int32_t& val) const noexcept = 0;
    virtual void Extract(int column, std::uint32_t& val) const noexcept = 0;
    virtual void Extract(int column, std::int64_t& val) const noexcept = 0;
    virtual void Extract(int column, std::uint64_t& val) const noexcept = 0;
    virtual void Extract(int column, float& val) const noexcept = 0;
    virtual void Extract(int column, double& val) const noexcept = 0;
    virtual void Extract(int column, std::string& val) const noexcept = 0;
    virtual void Extract(int column, std::vector<uint8_t>& val) const noexcept = 0;
};

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
