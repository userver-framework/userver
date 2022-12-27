#include <storages/mysql/impl/mysql_result.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

MySQLResult::MySQLResult() = default;

MySQLResult::MySQLResult(MySQLResult&& other) noexcept = default;

MySQLResult::~MySQLResult() = default;

void MySQLResult::AppendRow(MySQLRow&& row) { rows_.push_back(std::move(row)); }

std::size_t MySQLResult::RowsCount() const { return rows_.size(); }

const MySQLRow& MySQLResult::GetRow(std::size_t ind) const {
  UASSERT(ind < RowsCount());

  return rows_[ind];
}

MySQLRow& MySQLResult::GetRow(std::size_t ind) {
  UASSERT(ind < RowsCount());

  return rows_[ind];
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
