#pragma once

#include <cstddef>
#include <stdexcept>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

class Error : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class LogicError : public Error {
    using Error::Error;
};

class RuntimeError : public Error {
    using Error::Error;
};

class ConnectionError : public RuntimeError {
    using RuntimeError::RuntimeError;
};

class StatementError : public RuntimeError {
    using RuntimeError::RuntimeError;
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
