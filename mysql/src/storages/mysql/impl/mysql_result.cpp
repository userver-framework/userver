#include <storages/mysql/impl/mysql_result.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

MySQLResult::MySQLResult() = default;

MySQLResult::MySQLResult(MySQLResult&& other) noexcept = default;

MySQLResult::~MySQLResult() = default;

void MySQLResult::AppendRow(MySQLRow&& row) { rows_.push_back(std::move(row)); }

std::size_t MySQLResult::RowsCount() const { return rows_.size(); }

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
