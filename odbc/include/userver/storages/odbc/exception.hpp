#pragma once

#include <stdexcept>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

class Error : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class LogicError : public Error {
public:
    explicit LogicError(std::string msg);

    const char* what() const noexcept override;

private:
    std::string msg_;
};

class RuntimeError : public Error {
public:
    explicit RuntimeError(std::string msg);

    const char* what() const noexcept override;

private:
    std::string msg_;
};

class ConnectionError : public RuntimeError {
public:
    explicit ConnectionError(std::string msg);

    const char* what() const noexcept override;

private:
    std::string msg_;
};

class StatementError : public RuntimeError {
public:
    explicit StatementError(std::string msg);

    const char* what() const noexcept override;

private:
    std::string msg_;
};

class ResultSetError : public Error {
public:
    explicit ResultSetError(std::string msg);

    const char* what() const noexcept override;

private:
    std::string msg_;
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
