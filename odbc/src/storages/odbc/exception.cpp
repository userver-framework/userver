#include <userver/storages/odbc/exception.hpp>

#include <fmt/format.h>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

LogicError::LogicError(std::string msg) : Error(msg), msg_(std::move(msg)) {}

const char* LogicError::what() const noexcept { return msg_.c_str(); }

ResultSetError::ResultSetError(std::string msg) : Error(msg), msg_(std::move(msg)) {}

const char* ResultSetError::what() const noexcept { return msg_.c_str(); }

FieldIndexOutOfBounds::FieldIndexOutOfBounds(std::size_t index)
    : ResultSetError(fmt::format("Field index {} is out of bounds", index)) {}

RowIndexOutOfBounds::RowIndexOutOfBounds(std::size_t index)
    : ResultSetError(fmt::format("Row index {} is out of bounds", index)) {}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
