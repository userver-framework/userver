#pragma once

#include <chrono>
#include <functional>
#include <memory>

#include <boost/optional.hpp>

namespace storages {
namespace postgres {

class Transaction;
class ResultSet;

class Cluster;
using ClusterPtr = std::shared_ptr<Cluster>;

namespace detail {
class Connection;
class ConnectionPtr;
using ConnectionCallback = std::function<void(Connection*)>;

class ResultWrapper;
using ResultWrapperPtr = std::shared_ptr<const ResultWrapper>;
}  // namespace detail

using TimeoutType = std::chrono::milliseconds;

struct CommandControl {
  TimeoutType network;
  TimeoutType statement;

  constexpr CommandControl WithNetworkTimeout(TimeoutType n) const noexcept {
    return {n, statement};
  }

  constexpr CommandControl WithStatementTimeout(TimeoutType s) const noexcept {
    return {network, s};
  }

  bool operator==(CommandControl const& rhs) const {
    return network == rhs.network && statement == rhs.statement;
  }
};

using OptionalCommandControl = boost::optional<CommandControl>;

}  // namespace postgres
}  // namespace storages
