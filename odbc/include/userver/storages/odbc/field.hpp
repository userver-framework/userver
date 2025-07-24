#pragma once

#include <userver/storages/odbc/odbc_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

class Field {
public:
    using size_type = std::size_t;

    size_type RowIndex() const { return row_index_; }
    size_type FieldIndex() const { return field_index_; }

    bool IsNull() const;

    std::string GetString() const;
    int64_t GetInt64() const;
    int32_t GetInt32() const;
    double GetDouble() const;
    bool GetBool() const;

protected:
    friend class Row;

    Field() = default;

    Field(detail::ResultWrapperPtr res, size_type row, size_type col)
        : res_{std::move(res)}, row_index_{row}, field_index_{col} {}

private:
    detail::ResultWrapperPtr res_;
    size_type row_index_{0};
    size_type field_index_{0};
};

}  // namespace storages::odbc

USERVER_NAMESPACE_END
