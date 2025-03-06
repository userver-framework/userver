#include <userver/storages/sqlite/result_set.hpp>

#include <userver/storages/sqlite/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

ResultSet::ResultSet(std::shared_ptr<impl::ResultWrapper> pimpl)
    : pimpl_{std::move(pimpl)} {}

ResultSet::ResultSet(ResultSet&& other) noexcept = default;

ResultSet& ResultSet::operator=(ResultSet&&) noexcept = default;

ResultSet::~ResultSet() = default;

ExecutionResult ResultSet::AsExecutionResult() && {
  const int rows_affected = pimpl_->RowsAffected();
  const int last_insert_id = pimpl_->LastInsertRowId();

  ExecutionResult result{};
  result.rows_affected = rows_affected;
  result.last_insert_id = last_insert_id;
  return result;
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
