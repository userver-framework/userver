#pragma once

/// @file userver/storages/mysql/row_types.hpp
/// @brief Helper tags to disambiguate result extraction between row and field.

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

/// Used for extracting statement results as a single field.
struct FieldTag {};
/// Used for extracting statement results as rows.
struct RowTag {};

inline constexpr FieldTag kFieldTag;
inline constexpr RowTag kRowTag{};

}  // namespace storages::mysql

USERVER_NAMESPACE_END
