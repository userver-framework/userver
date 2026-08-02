#pragma once

/// @file userver/storages/odbc/parameter_store.hpp
/// @brief @copybrief storages::odbc::ParameterStore

#include <concepts>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include <userver/storages/odbc/impl/parameter.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

class Cluster;
class Transaction;
class BulkParameterStore;

/// @cond
namespace impl {

template <typename T>
struct IsParameterStoreValue {
private:
    using Value = std::remove_cv_t<T>;
    using Pointee = std::remove_pointer_t<Value>;
    using Element = std::remove_extent_t<Value>;

public:
    static constexpr bool value =
        std::integral<Value> || std::floating_point<Value> || std::is_enum_v<Value> ||
        std::same_as<Value, std::string> || std::same_as<Value, std::string_view> || std::same_as<Value, Bytes> ||
        std::same_as<Value, Date> || std::same_as<Value, Time> || std::same_as<Value, Timestamp> || kIsDecimal<Value> ||
        std::same_as<Value, std::nullptr_t> || std::same_as<Value, std::nullopt_t> ||
        (std::is_pointer_v<Value> && (std::same_as<Pointee, char> || std::same_as<Pointee, const char>)) ||
        (std::is_array_v<Value> && std::same_as<std::remove_cv_t<Element>, char>);
};

template <typename T>
struct IsParameterStoreValue<std::optional<T>> final : IsParameterStoreValue<std::remove_cv_t<T>> {};

template <typename T>
inline constexpr bool kIsParameterStoreValue = IsParameterStoreValue<std::remove_cvref_t<T>>::value;

}  // namespace impl
/// @endcond

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

    /// @brief Copies a parameter supported by the variadic ODBC API to the end
    /// of the ordered list.
    /// @returns `*this` for chained construction.
    template <typename T>
    requires(impl::kIsParameterStoreValue<T> && std::constructible_from<impl::Parameter, const T&>)
    ParameterStore& PushBack(const T& parameter) {
        parameters_.emplace_back(parameter);
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
