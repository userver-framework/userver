#include <storages/odbc/impl/query_result.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::impl {

QueryResult::QueryResult() = default;

QueryResult::~QueryResult() = default;

QueryResult::QueryResult(QueryResult&& other) noexcept
    : rows_(std::move(other.rows_)) {}

void QueryResult::AppendRow(QueryResultRow&& row) {
    rows_.push_back(std::move(row));
}

std::size_t QueryResult::RowsCount() const {
    return rows_.size();
}

const QueryResultRow& QueryResult::GetRow(std::size_t ind) const {
    return rows_.at(ind);
}

QueryResultRow& QueryResult::GetRow(std::size_t ind) {
    return rows_.at(ind);
}

}  // namespace storages::odbc::impl

USERVER_NAMESPACE_END 