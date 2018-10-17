#pragma once

#include "pg_impl_fwd.hpp"

namespace storages {
namespace postgres {

class ConnectionPool {
 public:
  /// Get idle connection from pool
  /// If no idle connection and max_pool_size is not reached - create a new
  /// connection.
  /// Suspends coroutine until a connection is available
  ConnectionPtr GetIdleConnection(/*timeout ?*/);
};

}  // namespace postgres
}  // namespace storages
