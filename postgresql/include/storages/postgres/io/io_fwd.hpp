#pragma once

namespace storages::postgres::io {

/// @brief Primary template for declaring mapping to a PostgreSQL system type.
template <typename T>
struct CppToSystemPg;

/// @brief Primary template for declaring mapping to a PostgreSQL user type.
///
/// Must contain a `static constexpr DBTypeName postgres_name` member.
///
/// For more information see @ref pg_user_types
///
/// For enumerated types must derive from EnumMappingBase template and have a
/// `static constexpr EnumeratorList enumerators` member. The EnumeratorList
/// is a type alias declared in EnumMappingBase.
///
/// For more information see @ref pg_enum
template <typename T>
struct CppToUserPg;

}  // namespace storages::postgres::io
