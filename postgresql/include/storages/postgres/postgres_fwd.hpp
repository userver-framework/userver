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

using TimeoutDuration = std::chrono::milliseconds;

struct CommandControl {
  TimeoutDuration network;
  TimeoutDuration statement;

  constexpr CommandControl() = default;

  constexpr CommandControl(TimeoutDuration network, TimeoutDuration statement)
      : network(network), statement(statement) {}

  constexpr CommandControl WithNetworkTimeout(TimeoutDuration n) const
      noexcept {
    return {n, statement};
  }

  constexpr CommandControl WithStatementTimeout(TimeoutDuration s) const
      noexcept {
    return {network, s};
  }

  bool operator==(CommandControl const& rhs) const {
    return network == rhs.network && statement == rhs.statement;
  }

  bool operator!=(const CommandControl& rhs) const { return !(*this == rhs); }
};

using OptionalCommandControl = boost::optional<CommandControl>;
using SharedCommandControl = std::shared_ptr<const CommandControl>;

}  // namespace postgres
}  // namespace storages
