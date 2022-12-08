#include <storages/mysql/impl/mysql_row.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

MySQLRow::MySQLRow() = default;

MySQLRow::MySQLRow(char** data, std::size_t fields_count,
                   std::size_t* fields_lengths) {
  UINVARIANT(fields_lengths != nullptr, "Parsing MySQL result broke");

  columns_.reserve(fields_count);
  for (size_t i = 0; i < fields_count; ++i) {
    columns_.emplace_back(data[i], fields_lengths[i]);
  }
}

MySQLRow::MySQLRow(MySQLRow&& other) noexcept = default;

MySQLRow::~MySQLRow() = default;

std::size_t MySQLRow::FieldsCount() const { return columns_.size(); }

std::string& MySQLRow::GetField(std::size_t ind) {
  UASSERT(ind < FieldsCount());

  return columns_[ind];
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
