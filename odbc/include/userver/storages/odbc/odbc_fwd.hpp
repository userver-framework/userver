#pragma once

/// @file userver/storages/odbc/odbc_fwd.hpp
/// @brief Forward declarations of some popular odbc related types

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

class ResultSet;
class Row;

class Cluster;

/// @brief Smart pointer to the storages::odbc::Cluster
using ClusterPtr = std::shared_ptr<Cluster>;

namespace detail {
class ResultWrapper;
using ResultWrapperPtr = std::shared_ptr<const ResultWrapper>;

class ConnectionPtr;

class Pool;
using PoolPtr = std::shared_ptr<Pool>;
}  // namespace detail

}  // namespace storages::odbc

USERVER_NAMESPACE_END
