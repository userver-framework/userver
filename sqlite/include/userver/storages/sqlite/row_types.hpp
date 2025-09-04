#pragma once

/// @file userver/storages/sqlite/row_types.hpp
/// @brief Helper tags to disambiguate result extraction between row and field.

#include <tuple>
#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

/// Used for extracting statement results as a single field.
struct FieldTag {};
/// Used for extracting statement results as rows.
struct RowTag {};

inline constexpr FieldTag kFieldTag;
inline constexpr RowTag kRowTag{};

template <typename T>
struct IsTuple : std::false_type {};

template <typename... Args>
struct IsTuple<std::tuple<Args...>> : std::true_type {};

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
