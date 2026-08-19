#pragma once

/// @file userver/storages/odbc/exception.hpp
/// @brief ODBC storage exception hierarchy

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

/// A single diagnostic record reported by the ODBC driver manager or driver.
struct DiagnosticRecord final {
    std::string sql_state;
    int native_error{0};
    std::string message;
};

class Error : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class LogicError : public Error {
    using Error::Error;
};

class RuntimeError : public Error {
public:
    using Error::Error;

    RuntimeError(std::string message, std::vector<DiagnosticRecord> diagnostics, bool invalid_handle = false);

    /// Structured driver diagnostics, in the order returned by ODBC.
    const std::vector<DiagnosticRecord>& GetDiagnostics() const noexcept;

    /// Whether any diagnostic has the specified two-character SQLSTATE class.
    bool HasSqlStateClass(std::string_view sql_state_class) const noexcept;

    /// Whether the failed ODBC call returned SQL_INVALID_HANDLE.
    bool IsInvalidHandle() const noexcept;

private:
    std::vector<DiagnosticRecord> diagnostics_;
    bool invalid_handle_{false};
};

class ConnectionError : public RuntimeError {
    using RuntimeError::RuntimeError;
};

class StatementError : public RuntimeError {
    using RuntimeError::RuntimeError;
};

/// Thrown when an ODBC pool cannot provide a connection for a non-timeout reason.
class PoolError : public RuntimeError {
    using RuntimeError::RuntimeError;
};

/// Thrown when the operation is aborted because an @ref engine::Deadline has expired
/// (including task-inherited request deadlines).
class OperationInterrupted : public RuntimeError {
    using RuntimeError::RuntimeError;
};

class TransactionException : public LogicError {
    using LogicError::LogicError;
};

class ResultSetError : public RuntimeError {
    using RuntimeError::RuntimeError;
};

class RowIndexOutOfBounds : public ResultSetError {
public:
    explicit RowIndexOutOfBounds(std::size_t index);
};

class FieldIndexOutOfBounds : public ResultSetError {
public:
    explicit FieldIndexOutOfBounds(std::size_t index);
};

}  // namespace storages::odbc

USERVER_NAMESPACE_END
