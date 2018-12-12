#pragma once

namespace storages::postgres::io {

/// @brief Primary template for declaring mapping to a postgres system type.
template <typename T>
struct CppToSystemPg;

/// @brief Primary template for declaring mapping to a postgres user type.
/// @see psql_user_types
template <typename T>
struct CppToUserPg;

}  // namespace storages::postgres::io
