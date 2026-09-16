#pragma once

/// @file userver/ydb/query.hpp
/// @brief YDB SQL query type alias

#include <userver/storages/query.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

/// @brief YDB SQL query type alias for @ref storages::Query
using Query = storages::Query;

}  // namespace ydb

USERVER_NAMESPACE_END
