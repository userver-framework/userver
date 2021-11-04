#pragma once

/// @file userver/storages/postgres/postgres_fwd.hpp
/// @brief Forward declarations of some popular postgre related types

#include <chrono>
#include <functional>
#include <memory>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

class Transaction;
class ResultSet;
class Row;

class Cluster;

/// @brief Smart pointer to the storages::postgres::Cluster
using ClusterPtr = std::shared_ptr<Cluster>;

namespace detail {
class Connection;
class ConnectionImpl;
class ConnectionPtr;
using ConnectionCallback = std::function<void(Connection*)>;

class ResultWrapper;
using ResultWrapperPtr = std::shared_ptr<const ResultWrapper>;
}  // namespace detail

using TimeoutDuration = std::chrono::milliseconds;

class DefaultCommandControls;

}  // namespace storages::postgres

USERVER_NAMESPACE_END
