#pragma once

#include <memory>

namespace storages {
namespace postgres {

class Transaction;
class ResultSet;

class Cluster;
using ClusterPtr = std::shared_ptr<Cluster>;

namespace detail {
class Connection;
using ConnectionCallback = std::function<void(Connection*)>;
using ConnectionPtr = std::unique_ptr<Connection, ConnectionCallback>;

class ResultWrapper;
using ResultWrapperPtr = std::shared_ptr<const ResultWrapper>;
}  // namespace detail

}  // namespace postgres
}  // namespace storages
