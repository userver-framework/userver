#include <storages/odbc/detail/result_wrapper.hpp>
#include <userver/storages/odbc/exception.hpp>
#include <userver/storages/odbc/row.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

Row::size_type Row::Size() const { return res_->FieldCount(); }

Row::reference Row::operator[](size_type index) const {
    if (index >= Size()) throw FieldIndexOutOfBounds{index};
    return {res_, row_index_, index};
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
