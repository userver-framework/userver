#pragma once

namespace storages::postgres::io {

/// @brief Primary template for declaring
template <typename T>
struct CppToSystemPg;

template <typename T>
struct CppToUserPg;

}  // namespace storages::postgres::io
