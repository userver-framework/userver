#pragma once

#include <userver/storages/odbc/field.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

class Row {
public:
    using size_type = std::size_t;

    using value_type = Field;
    using reference = Field;

    size_type Size() const;

    reference operator[](size_type index) const;

protected:
    friend class ResultSet;

    Row() = default;

    Row(detail::ResultWrapperPtr res, size_type row_index) : res_{std::move(res)}, row_index_(row_index) {}

private:
    detail::ResultWrapperPtr res_;
    size_type row_index_{0};
};

}  // namespace storages::odbc

USERVER_NAMESPACE_END
