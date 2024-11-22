#pragma once

/// @file userver/storages/sqlite/row_types.hpp
/// @brief Helper tags to disambiguate result extraction between row and field.

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

/// Used for extracting statement results as a single field.
struct FieldTag {};
/// Used for extracting statement results as rows.
struct RowTag {};

inline constexpr FieldTag kFieldTag;
inline constexpr RowTag kRowTag{};

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
