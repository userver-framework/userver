#pragma once

#include <stdexcept>

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

class ResultSetError : public Error {
public:
    ResultSetError(std::string msg);

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
