#include <userver/storages/sqlite/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

ResultSet::ResultSet() = default;

ResultSet::size_type ResultSet::Size() const {
  return 0;
}

ResultSet::size_type ResultSet::RowsAffected() const {
  return 0;
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
