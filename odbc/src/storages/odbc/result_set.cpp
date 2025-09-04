
#include <userver/storages/odbc/result_set.hpp>

#include <userver/storages/odbc/exception.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

ResultSet::size_type ResultSet::Size() const { return pimpl_ != nullptr ? pimpl_->RowCount() : 0; }

ResultSet::size_type ResultSet::FieldCount() const { return pimpl_ != nullptr ? pimpl_->FieldCount() : 0; }

bool ResultSet::IsEmpty() const { return Size() == 0; }

ResultSet::reference ResultSet::operator[](size_type index) const& {
    if (index >= Size()) throw RowIndexOutOfBounds{index};
    return {pimpl_, index};
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
