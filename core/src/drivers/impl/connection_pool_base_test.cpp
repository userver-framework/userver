#include <userver/utest/utest.hpp>

#include <userver/drivers/impl/connection_pool_base.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/wait_all_checked.hpp>

USERVER_NAMESPACE_BEGIN

namespace drivers::impl {

namespace {

class DummyConnection final {
 public:
  static bool IsBroken() { return false; }
};

struct DummyMetrics {
  virtual void AccountConnectionCreated() {}
  virtual void AccountConnectionDestroyed() noexcept {}
  virtual void AccountConnectionAcquired() {}
  virtual void AccountConnectionReleased() {}
  virtual void AccountOverload() {}
};

template <typename Derived, typename Connection = DummyConnection>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class DummyBasePool : public ConnectionPoolBase<Connection, Derived>,
                      public DummyMetrics {
 public:
  DummyBasePool(std::size_t max_pool_size,
                std::size_t max_simultaneously_connecting_clients)
      : ConnectionPoolBase<Connection, Derived>{
            max_pool_size, max_simultaneously_connecting_clients} {}

  void InitPool(std::size_t initial_pool_size) {
    ConnectionPoolBase<Connection, Derived>::Init(
        initial_pool_size, std::chrono::milliseconds{10});
  }
};

class ThrowingPool final : public DummyBasePool<ThrowingPool> {
 public:
  using DummyBasePool<ThrowingPool>::DummyBasePool;

  ~ThrowingPool() { Reset(); }

 private:
  friend class ConnectionPoolBase<DummyConnection, ThrowingPool>;

  static ConnectionUniquePtr DoCreateConnection(engine::Deadline) {
    throw std::runtime_error{"oops"};
  }
};

class BasicPool final : public DummyBasePool<BasicPool> {
 public:
  using DummyBasePool<BasicPool>::DummyBasePool;

  ~BasicPool() { Reset(); }

  struct ConnectionPtr final {
    std::shared_ptr<BasicPool> pool;
    std::unique_ptr<DummyConnection> connection;

    ConnectionPtr(std::shared_ptr<BasicPool>&& pool,
                  std::unique_ptr<DummyConnection>&& connection)
        : pool{std::move(pool)}, connection{std::move(connection)} {}

    ConnectionPtr(ConnectionPtr&& other) noexcept = default;

    ~ConnectionPtr() {
      if (pool) {
        pool->ReleaseConnection(std::move(connection));
      }
    }
  };

  ConnectionPtr Acquire(engine::Deadline deadline) {
    auto holder = AcquireConnection(deadline);

    return {std::move(holder.pool_ptr), std::move(holder.connection_ptr)};
  }

  void AddConnectionIntoPool() { PushConnection({}); };

  std::size_t AliveConnectionsCount() const {
    return AliveConnectionsCountApprox();
  }

  std::size_t GetOverloadCount() const { return overloaded_; }

  void ForceNewConnectionsToTimeout() { force_connection_timeout_ = true; }

 private:
  friend class ConnectionPoolBase<DummyConnection, BasicPool>;

  ConnectionUniquePtr DoCreateConnection(engine::Deadline) const {
    if (force_connection_timeout_) {
      engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
    }

    return std::make_unique<DummyConnection>();
  }

  void AccountOverload() override { ++overloaded_; }

  bool force_connection_timeout_{false};
  std::atomic<std::size_t> overloaded_{0};
};

class BrokenConnection final {
 public:
  static bool IsBroken() { return true; }

  BrokenConnection(std::atomic<std::size_t>& destruction_counter)
      : destruction_counter_{destruction_counter} {}

  ~BrokenConnection() { ++destruction_counter_; }

 private:
  std::atomic<std::size_t>& destruction_counter_;
};

class BrokenConnectionsPool final
    : public DummyBasePool<BrokenConnectionsPool, BrokenConnection> {
 public:
  using DummyBasePool<BrokenConnectionsPool, BrokenConnection>::DummyBasePool;

  ~BrokenConnectionsPool() { Reset(); }

  std::size_t GetCreatedMetric() const { return created_connections_metric_; }

  std::size_t GetDestroyedMetric() const {
    return destroyed_connections_metric_;
  }

  std::size_t GetAcquiredMetric() const { return acquired_connections_metric_; }

  std::size_t GetReleasedMetric() const { return released_connections_metric_; }

  std::size_t GetDestroyedCount() const { return destroyed_connections_count_; }

  void AcquireAndRelease() {
    auto pool_and_connection = AcquireConnection({});
    ReleaseConnection(std::move(pool_and_connection.connection_ptr));
  }

  ConnectionUniquePtr Acquire() { return AcquireConnection({}).connection_ptr; }

  void Release(ConnectionUniquePtr connection_ptr) {
    ReleaseConnection(std::move(connection_ptr));
  }

 private:
  friend class ConnectionPoolBase<BrokenConnection, BrokenConnectionsPool>;

  ConnectionUniquePtr DoCreateConnection(engine::Deadline) {
    return std::make_unique<BrokenConnection>(destroyed_connections_count_);
  }

  void AccountConnectionCreated() override { ++created_connections_metric_; }
  void AccountConnectionDestroyed() noexcept override {
    ++destroyed_connections_metric_;
  }
  void AccountConnectionAcquired() override { ++acquired_connections_metric_; }
  void AccountConnectionReleased() override { ++released_connections_metric_; }

  std::atomic<std::size_t> destroyed_connections_count_{0};

  std::atomic<std::size_t> created_connections_metric_{0};
  std::atomic<std::size_t> destroyed_connections_metric_{0};
  std::atomic<std::size_t> acquired_connections_metric_{0};
  std::atomic<std::size_t> released_connections_metric_{0};
};

template <typename DerivedPool>
std::shared_ptr<DerivedPool> CreatePool(
    std::size_t initial_pool_size = 5, std::size_t max_pool_size = 5,
    std::size_t max_simultaneously_connecting_clients = 2) {
  auto pool = std::make_shared<DerivedPool>(
      max_pool_size, max_simultaneously_connecting_clients);

  pool->InitPool(initial_pool_size);
  return pool;
}

}  // namespace

UTEST(ConnectionPoolBase, RethrowsFromInit) {
  EXPECT_ANY_THROW(CreatePool<ThrowingPool>());
}

UTEST(ConnectionPoolBase, Basic) { UEXPECT_NO_THROW(CreatePool<BasicPool>()); }

UTEST(ConnectionPoolBase, HonorsMaxPoolSize) {
  constexpr std::size_t max_pool_size = 4;
  auto pool = CreatePool<BasicPool>(max_pool_size, max_pool_size);

  std::vector<BasicPool::ConnectionPtr> connections;
  connections.reserve(max_pool_size);

  const auto acquire = [&pool] {
    return pool->Acquire(
        engine::Deadline::FromDuration(std::chrono::milliseconds{2}));
  };

  for (std::size_t i = 0; i < max_pool_size; ++i) {
    connections.push_back(acquire());
  }

  // At this point the pool is exhausted (all available connections are given
  // away), so this is expected to fail to acquire a semaphore.
  UEXPECT_THROW(acquire(), PoolWaitLimitExceededError);

  connections.pop_back();
  // Now, however, there is one free connection.
  EXPECT_NO_THROW(acquire());
}

UTEST(ConnectionPoolBase, HonorsMaxSimultaneosulyConnectingClients) {
  constexpr std::size_t max_connecting = 3;
  auto pool = CreatePool<BasicPool>(0, max_connecting + 1, max_connecting);

  // Now creating a connection takes forever
  pool->ForceNewConnectionsToTimeout();

  std::vector<engine::TaskWithResult<void>> acquire_tasks;
  acquire_tasks.reserve(max_connecting + 1);
  // And we try to connect in parallel, with higher degree of parallelism
  // than what pool allows
  for (std::size_t i = 0; i < max_connecting + 1; ++i) {
    acquire_tasks.push_back(engine::AsyncNoSpan([&pool] {
      pool->Acquire(
          // this deadline is ignored in DummyConnection creation, and it only
          // affects semaphores in the pool
          engine::Deadline::FromDuration(std::chrono::milliseconds{10}));
    }));
  }

  // Don't get confused by 10ms deadline - the deadline is actually ignored in
  // connection creation, this throws because we failed to acquire the
  // 'connecting' semaphore lock.
  UASSERT_THROW(engine::WaitAllChecked(acquire_tasks),
                PoolWaitLimitExceededError);
  EXPECT_EQ(pool->GetOverloadCount(), 1);
}

UTEST(ConnectionPoolBase, HonorsMaxSimultaneosulyConnectingClientsInInit) {
  constexpr std::size_t max_connecting = 3;
  auto pool = std::make_shared<BasicPool>(max_connecting + 1, max_connecting);

  // Init hasn't been called yet, so let's make a connection take forever to
  // construct, and validate that Init throws some overload exception.
  pool->ForceNewConnectionsToTimeout();

  // The deadline in Init is ignored in actual connection construction (in tests
  // that is), thus we are indeed checking the connecting semaphore acquire
  // timeout.
  UEXPECT_THROW(pool->InitPool(max_connecting + 1), PoolWaitLimitExceededError);
}

UTEST(ConnectionPoolBase, DestroysBrokenConnections) {
  auto pool = CreatePool<BrokenConnectionsPool>();

  constexpr std::size_t iterations = 10;
  for (std::size_t i = 0; i < iterations; ++i) {
    // We acquire a connection and release it immediately, since it's broken
    // (conn->IsBroken() is always true) it gets destroyed right away.
    pool->AcquireAndRelease();
  }

  const auto destroyed_metric = pool->GetDestroyedMetric();
  const auto actually_destroyed = pool->GetDestroyedCount();
  EXPECT_EQ(destroyed_metric, iterations);
  EXPECT_EQ(actually_destroyed, iterations);
}

UTEST(ConnectionPoolBase, HonorsQueueMaxSize) {
  constexpr std::size_t max_size = 5;
  auto pool = CreatePool<BasicPool>(0 /* initial_pool_size */, max_size);

  std::vector<BasicPool::ConnectionPtr> connections;
  connections.reserve(max_size);
  for (std::size_t i = 0; i < max_size; ++i) {
    connections.push_back(pool->Acquire({}));
  }
  EXPECT_EQ(pool->AliveConnectionsCount(), max_size);
  pool->AddConnectionIntoPool();
  EXPECT_EQ(pool->AliveConnectionsCount(), max_size + 1);

  connections.clear();
  // Now that we've returned all the connections pool should have max_size
  // connections even though we've pushed a new one explicitly
  EXPECT_EQ(pool->AliveConnectionsCount(), max_size);
}

UTEST(ConnectionPoolBase, Metrics) {
  auto pool = CreatePool<BrokenConnectionsPool>(0, 5, 1);
  EXPECT_EQ(pool->GetCreatedMetric(), 0);

  constexpr std::size_t acq_rel_iterations = 3;
  for (std::size_t i = 0; i < acq_rel_iterations; ++i) {
    pool->AcquireAndRelease();
  }
  EXPECT_EQ(pool->GetCreatedMetric(), acq_rel_iterations);
  EXPECT_EQ(pool->GetDestroyedMetric(), acq_rel_iterations);
  EXPECT_EQ(pool->GetAcquiredMetric(), acq_rel_iterations);
  EXPECT_EQ(pool->GetReleasedMetric(), acq_rel_iterations);

  constexpr std::size_t acquire_iterations = 2;
  std::vector<std::unique_ptr<BrokenConnection>> connections;
  connections.reserve(acquire_iterations);
  for (std::size_t i = 0; i < acquire_iterations; ++i) {
    connections.push_back(pool->Acquire());
  }
  EXPECT_EQ(pool->GetCreatedMetric(), acq_rel_iterations + acquire_iterations);
  EXPECT_EQ(pool->GetDestroyedMetric(), acq_rel_iterations);
  EXPECT_EQ(pool->GetAcquiredMetric(), acq_rel_iterations + acquire_iterations);
  EXPECT_EQ(pool->GetReleasedMetric(), acq_rel_iterations);

  for (auto& connection : connections) {
    pool->Release(std::move(connection));
  }
  constexpr auto total_iterations = acq_rel_iterations + acquire_iterations;
  EXPECT_EQ(pool->GetCreatedMetric(), total_iterations);
  EXPECT_EQ(pool->GetDestroyedMetric(), total_iterations);
  EXPECT_EQ(pool->GetAcquiredMetric(), total_iterations);
  EXPECT_EQ(pool->GetReleasedMetric(), total_iterations);
}

}  // namespace drivers::impl

USERVER_NAMESPACE_END
