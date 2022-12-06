#include <storages/mysql/impl/mysql_row.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

MySQLRow::MySQLRow() = default;

MySQLRow::MySQLRow(char** data, std::size_t fields_count) {
  columns_.reserve(fields_count);
  for (size_t i = 0; i < fields_count; ++i) {
    columns_.emplace_back(data[i]);
  }
}

MySQLRow::MySQLRow(MySQLRow&& other) noexcept = default;

MySQLRow::~MySQLRow() = default;

std::size_t MySQLRow::FieldsCount() const { return columns_.size(); }

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
