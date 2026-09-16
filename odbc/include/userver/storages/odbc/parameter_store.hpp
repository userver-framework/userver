#pragma once

/// @file userver/storages/odbc/parameter_store.hpp
/// @brief @copybrief storages::odbc::ParameterStore

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <type_traits>

#include <userver/storages/odbc/impl/parameter.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

class Cluster;
class Transaction;
class BulkParameterStore;

/// @ingroup userver_containers
///
/// @brief Owning, ordered list of dynamically assembled ODBC parameters.
///
/// Values are copied into the store and remain valid independently of the
/// source objects. Use an empty `std::optional<T>` for SQL NULL: `T` determines
/// the parameter type used for ODBC binding. Raw `nullptr` and `std::nullopt`
/// remain untyped, just like in the variadic API, and should only be used when
/// the driver can infer the type from the statement. A null `const char*` is a
/// typed string NULL.
///
/// @warning Parameters are always values for existing `?` placeholders. Never
/// interpolate them into the SQL query text.
class ParameterStore final {
public:
    ParameterStore() = default;
    ParameterStore(const ParameterStore&) = delete;
    ParameterStore(ParameterStore&&) noexcept = default;
    ParameterStore& operator=(const ParameterStore&) = delete;
    ParameterStore& operator=(ParameterStore&&) noexcept = default;

    /// @brief Copies a scalar parameter or the declaration-order fields of a
    /// supported aggregate to the end of the ordered list.
    /// @returns `*this` for chained construction.
    template <typename T>
    requires impl::kIsParameterArgument<T>
    ParameterStore& PushBack(const T& parameter) {
        auto appended = impl::MakeParameterList(parameter);
        static_assert(std::is_nothrow_move_constructible_v<impl::Parameter>);
        if (appended.size() > parameters_.max_size() - parameters_.size()) {
            throw std::length_error("ODBC ParameterStore size exceeds its maximum");
        }
        parameters_.reserve(parameters_.size() + appended.size());
        std::move(appended.begin(), appended.end(), std::back_inserter(parameters_));
        return *this;
    }

    /// Returns whether the parameter list is empty.
    bool IsEmpty() const noexcept { return parameters_.empty(); }

    /// Returns the number of stored parameters.
    std::size_t Size() const noexcept { return parameters_.size(); }

private:
    friend class Cluster;
    friend class Transaction;
    friend class BulkParameterStore;

    const impl::ParameterList& GetParameters() const noexcept { return parameters_; }

    impl::ParameterList parameters_;
};

}  // namespace storages::odbc

USERVER_NAMESPACE_END
