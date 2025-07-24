#include <userver/storages/odbc/exception.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

FieldIndexOutOfBounds::FieldIndexOutOfBounds(std::size_t index)
    : ResultSetError(fmt::format("Field index {} is out of bounds", index)) {}

RowIndexOutOfBounds::RowIndexOutOfBounds(std::size_t index)
    : ResultSetError(fmt::format("Row index {} is out of bounds", index)) {}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
