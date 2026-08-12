#pragma once

/// @file userver/storages/clickhouse/io/io_fwd.hpp
/// @brief Customization point for Ch driver C++ <-> ClickHouse mappings.

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io {

template <typename T>
struct CppToClickhouse;

}  // namespace storages::clickhouse::io

USERVER_NAMESPACE_END
