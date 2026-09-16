#pragma once

/// @file userver/storages/odbc/io/type_mapping.hpp
/// @brief Explicit C++ conversions to and from ODBC bound scalar types.

#include <concepts>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include <userver/storages/odbc/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::io {

/// @brief Customization point for mapping a user type to one ODBC scalar.
///
/// Specializations declare a cv-unqualified, non-reference `BoundType` and
/// may independently provide `static BoundType ToOdbc(const T&)` for query
/// parameters and `static T FromOdbc(BoundType)` for result fields.
template <typename T>
struct CppToOdbc;

/// @cond
namespace traits {

template <typename T>
struct IsOptional : std::false_type {};

template <typename T>
struct IsOptional<std::optional<T>> : std::true_type {
    using ValueType = T;
};

template <typename T>
inline constexpr bool kIsOptional = IsOptional<std::remove_cvref_t<T>>::value;

template <typename T>
inline constexpr bool kIsDirectBoundType =
    std::same_as<T, std::remove_cvref_t<T>> && !kIsOptional<T> &&
    (std::same_as<T, bool> || (std::integral<T> && !std::same_as<T, bool> && sizeof(T) <= sizeof(std::uint64_t)) ||
     std::same_as<T, float> || std::same_as<T, double> || std::same_as<T, std::string> || std::same_as<T, Bytes> ||
     std::same_as<T, Date> || std::same_as<T, Time> || std::same_as<T, Timestamp> ||
     storages::odbc::impl::kIsDecimal<T>);

template <typename T>
inline constexpr bool kHasMappingDeclaration = requires { sizeof(CppToOdbc<std::remove_cvref_t<T>>); };

template <typename T>
constexpr bool HasValidBoundType() {
    using Value = std::remove_cvref_t<T>;
    if constexpr (!kHasMappingDeclaration<Value> || kIsOptional<Value>) {
        return false;
    } else if constexpr (requires { typename CppToOdbc<Value>::BoundType; }) {
        return kIsDirectBoundType<typename CppToOdbc<Value>::BoundType>;
    }

    return false;
}

template <typename T>
inline constexpr bool kHasValidBoundType = HasValidBoundType<std::remove_cvref_t<T>>();

template <typename T>
constexpr bool HasToOdbc() {
    using Value = std::remove_cvref_t<T>;
    if constexpr (!kHasValidBoundType<Value>) {
        return false;
    } else {
        using Mapping = CppToOdbc<Value>;
        using BoundType = typename Mapping::BoundType;
        return requires { static_cast<BoundType (*)(const Value&)>(&Mapping::ToOdbc); };
    }
}

template <typename T>
inline constexpr bool kHasToOdbc = HasToOdbc<std::remove_cvref_t<T>>();

template <typename T>
constexpr bool HasFromOdbc() {
    using Value = std::remove_cvref_t<T>;
    if constexpr (!kHasValidBoundType<Value>) {
        return false;
    } else {
        using Mapping = CppToOdbc<Value>;
        using BoundType = typename Mapping::BoundType;
        return requires { static_cast<Value (*)(BoundType)>(&Mapping::FromOdbc); };
    }
}

template <typename T>
inline constexpr bool kHasFromOdbc = HasFromOdbc<std::remove_cvref_t<T>>();

template <typename T>
using BoundType = typename CppToOdbc<std::remove_cvref_t<T>>::BoundType;

template <typename Derived>
struct NonBaseInitializer final {
    template <typename Type>
    requires(!std::is_base_of_v<std::remove_cvref_t<Type>, Derived>)
    operator Type() const noexcept {  // NOLINT(google-explicit-constructor)
        std::abort();
    }
};

template <typename Value, std::size_t... Index>
constexpr bool IsNonBaseAggregateInitializable(std::index_sequence<Index...>) {
    return requires { Value{(static_cast<void>(Index), NonBaseInitializer<Value>{})...}; };
}

// Matches the maximum aggregate arity supported by Boost.PFR's generated core17 implementation.
inline constexpr std::size_t kPfrMaxFields = 200;

template <typename Value, std::size_t... Index>
constexpr bool HasNonBaseAggregateArity(std::index_sequence<Index...>) {
    return (IsNonBaseAggregateInitializable<Value>(std::make_index_sequence<Index + 1>{}) || ...);
}

template <typename T>
constexpr bool AggregateHasNoBaseClass() {
    using Value = std::remove_cvref_t<T>;
    if constexpr (!std::is_aggregate_v<Value> || std::is_empty_v<Value>) {
        return false;
    } else {
        return HasNonBaseAggregateArity<Value>(std::make_index_sequence<kPfrMaxFields>{});
    }
}

template <typename T>
inline constexpr bool kAggregateHasNoBaseClass = AggregateHasNoBaseClass<T>();

}  // namespace traits
/// @endcond

}  // namespace storages::odbc::io

USERVER_NAMESPACE_END
