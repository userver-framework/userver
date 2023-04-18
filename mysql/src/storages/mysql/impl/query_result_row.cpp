#include <storages/mysql/impl/query_result_row.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

QueryResultRow::QueryResultRow() = default;

QueryResultRow::QueryResultRow(char** data, std::size_t fields_count,
                               std::size_t* fields_lengths) {
  UINVARIANT(fields_lengths != nullptr, "Parsing MySQL result broke");

  columns_.reserve(fields_count);
  for (size_t i = 0; i < fields_count; ++i) {
    columns_.emplace_back(data[i], fields_lengths[i]);
  }
}

QueryResultRow::QueryResultRow(QueryResultRow&& other) noexcept = default;

QueryResultRow::~QueryResultRow() = default;

std::size_t QueryResultRow::FieldsCount() const { return columns_.size(); }

const std::string& QueryResultRow::GetField(std::size_t ind) const {
  UASSERT(ind < FieldsCount());

  return columns_[ind];
}

std::string& QueryResultRow::GetField(std::size_t ind) {
  UASSERT(ind < FieldsCount());

  return columns_[ind];
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
