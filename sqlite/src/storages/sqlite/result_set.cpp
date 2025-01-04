#include <userver/storages/sqlite/result_set.hpp>

#include <userver/storages/sqlite/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

ResultSet::ResultSet() = default;

ResultSet::size_type ResultSet::Size() const { return 0; }

ResultSet::size_type ResultSet::RowsAffected() const { return 0; }

ResultSet::const_iterator ResultSet::cbegin() const& { return {}; }

ResultSet::const_iterator ResultSet::cend() const& { return {}; }

ResultSet::const_reverse_iterator ResultSet::crbegin() const& { return {}; }

ResultSet::const_reverse_iterator ResultSet::crend() const& { return {}; }

ResultSet::reference ResultSet::Front() const& { return (*this)[0]; }

ResultSet::reference ResultSet::Back() const& { return (*this)[Size() - 1]; }

ResultSet::reference ResultSet::operator[](size_type index) const& {
  if (index >= Size()) throw SQLiteException{"Result set index out of range "};
  return {};
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
