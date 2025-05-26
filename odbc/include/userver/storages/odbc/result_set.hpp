#pragma once

#include <limits>
#include <memory>

#include <storages/odbc/detail/result_wrapper.hpp>
#include <userver/storages/odbc/row.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

/// @brief Result set for ODBC query execution
class ResultSet final {
public:
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    static constexpr size_type npos = std::numeric_limits<size_type>::max();

    //@{
    /** @name Row container concept */

    using value_type = Row;
    using reference = value_type;
    //@}

    explicit ResultSet(std::shared_ptr<detail::ResultWrapper> pimpl) : pimpl_{std::move(pimpl)} {}

    /// @brief Get the number of columns in the result set
    size_type FieldCount() const;

    size_type Size() const;

    /// @brief Check if the result set is empty
    bool IsEmpty() const;

    reference operator[](size_type index) const&;

private:
    std::shared_ptr<detail::ResultWrapper> pimpl_;
};

}  // namespace storages::odbc

USERVER_NAMESPACE_END
