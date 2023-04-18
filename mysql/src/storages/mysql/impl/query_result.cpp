#include <storages/mysql/impl/query_result.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

QueryResult::QueryResult() = default;

QueryResult::QueryResult(QueryResult&& other) noexcept = default;

QueryResult::~QueryResult() = default;

void QueryResult::AppendRow(QueryResultRow&& row) {
  rows_.push_back(std::move(row));
}

std::size_t QueryResult::RowsCount() const { return rows_.size(); }

const QueryResultRow& QueryResult::GetRow(std::size_t ind) const {
  UASSERT(ind < RowsCount());

  return rows_[ind];
}

QueryResultRow& QueryResult::GetRow(std::size_t ind) {
  UASSERT(ind < RowsCount());

  return rows_[ind];
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
