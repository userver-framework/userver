#pragma once

/// @file userver/drivers/impl/connection_pool_base.hpp
/// @brief @copybrief drivers::impl::ConnectionPoolBase

#include <atomic>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include <boost/lockfree/queue.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/get_all.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace drivers::impl {

/// @brief Thrown when no connection could be acquired from the pool within
/// specified timeout.
class PoolWaitLimitExceededError : public std::runtime_error {
 public:
  PoolWaitLimitExceededError();
};

/// @brief Base connection pool implementation to be derived in different
/// drivers. Takes care of synchronization, pool limits (min/max, simultaneously
/// connecting etc.) and provides hooks for metrics.
template <class Connection, class Derived>
class ConnectionPoolBase : public std::enable_shared_from_this<Derived> {
 public:
 protected:
  using ConnectionRawPtr = Connection*;
  using ConnectionUniquePtr = std::unique_ptr<Connection>;

  struct ConnectionHolder final {
    std::shared_ptr<Derived> pool_ptr;
    ConnectionUniquePtr connection_ptr;
  };

  /// @brief Constructor, doesn't create any connections, one should call `Init`
  /// to initialize the pool.
  ConnectionPoolBase(std::size_t max_pool_size,
                     std::size_t max_simultaneously_connecting_clients);
  /// @brief Destructor. One should call `Reset` before the destructor is
  /// invoked.
  ~ConnectionPoolBase();

  /// @brief Initializes the pool with given limits.
  /// Uses some methods that must or may be overridden in derived class, hence
  /// a separate method and not called from base class constructor.
  /// Derived class constructor is a good place to call this.
  void Init(std::size_t initial_size,
            std::chrono::milliseconds connection_setup_timeout);
  /// @brief Resets the pool, destroying all managed connections.
  /// Uses some methods that may be overridden in derived class, hence
  /// a separate function and not called from base class constructor.
  /// Derived class destructor is a good place to call this.
  void Reset();

  /// @brief Acquires a connection from the pool.
  ConnectionHolder AcquireConnection(engine::Deadline deadline);
  /// @brief Returns the connection to the pool.
  /// If `connection_ptr->IsBroken()` is true, the connection is destroyed.
  void ReleaseConnection(ConnectionUniquePtr connection_ptr);

  /// @brief Pops a connection from the pool, might create a new connection if
  /// there are no ready connections.
  ConnectionUniquePtr Pop(engine::Deadline deadline);
  /// @brief Tries to pop a ready connection from the pool,
  /// may return `nullptr`.
  ConnectionUniquePtr TryPop();

  /// @brief Returns the connection to the pool, internal.
  /// If `connection_ptr->IsBroken()` is true, the connection is destroyed.
  /// Doesn't affect pool limits, so you shouldn't call this directly,
  /// unless the connection is acquired from `TryPop` - then it's the only
  /// correct way to return it back.
  void DoRelease(ConnectionUniquePtr connection_ptr);

  /// @brief Creates a new connection and tries to push it into the ready
  /// queue. If the queue is full, the connection is dropped immediately.
  void PushConnection(engine::Deadline deadline);
  /// @brief Drops the connections - destroys the object and accounts for that.
  void Drop(ConnectionRawPtr connection_ptr) noexcept;

  /// @brief Returns the approximate count of alive connections (given away and
  /// ready to use).
  std::size_t AliveConnectionsCountApprox() const;

  /// @brief Call this method if for some reason a connection previously
  /// acquired from the pool won't be returned into it.
  /// If one fails to do so pool limits might shrink until pool becomes
  /// unusable.
  void NotifyConnectionWontBeReleased();

 private:
  Derived& AsDerived() noexcept;
  const Derived& AsDerived() const noexcept;

  ConnectionUniquePtr CreateConnection(
      const engine::SemaphoreLock& connecting_lock, engine::Deadline deadline);

  void CleanupQueue();

  void EnsureInitialized() const;
  void EnsureReset() const;

  engine::Semaphore given_away_semaphore_;
  engine::Semaphore connecting_semaphore_;

  boost::lockfree::queue<ConnectionRawPtr> queue_;
  std::atomic<std::size_t> alive_connections_{0};

  bool initialized_{false};
  bool reset_{false};
};

template <class Connection, class Derived>
ConnectionPoolBase<Connection, Derived>::ConnectionPoolBase(
    std::size_t max_pool_size,
    std::size_t max_simultaneously_connecting_clients)
    : given_away_semaphore_{max_pool_size},
      connecting_semaphore_{max_simultaneously_connecting_clients},
      queue_{max_pool_size} {}

template <class Connection, class Derived>
ConnectionPoolBase<Connection, Derived>::~ConnectionPoolBase() {
  if (initialized_) {
    // We don't call Reset here, because dropping a connection (when cleaning
    // up the queue) might invoke virtual methods, and the derived class is
    // already destroyed, so unexpected things could happen.
    // However, we assert that derived class performed a cleanup itself.
    EnsureReset();
  }
}

template <class Connection, class Derived>
void ConnectionPoolBase<Connection, Derived>::Init(
    std::size_t initial_size,
    std::chrono::milliseconds connection_setup_timeout) {
  UASSERT_MSG(!initialized_, "Calling Init multiple times is a API misuse");
  // We mark the pool as initialized even if this method throws, because
  // this `initialized_` field is here just to ensure correct API usage,
  // doesn't have much meaning aside from that.
  initialized_ = true;

  std::vector<engine::TaskWithResult<void>> init_tasks{};
  init_tasks.reserve(initial_size);

  for (std::size_t i = 0; i < initial_size; ++i) {
    init_tasks.push_back(engine::AsyncNoSpan([this, connection_setup_timeout] {
      PushConnection(engine::Deadline::FromDuration(connection_setup_timeout));
    }));
  }

  try {
    engine::GetAll(init_tasks);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "Failed to properly setup connection pool: " << ex;
    throw;
  }
}

template <class Connection, class Derived>
void ConnectionPoolBase<Connection, Derived>::Reset() {
  UASSERT_MSG(!reset_, "Calling Reset multiple times is a API misuse");
  reset_ = true;

  CleanupQueue();
}

template <class Connection, class Derived>
typename ConnectionPoolBase<Connection, Derived>::ConnectionHolder
ConnectionPoolBase<Connection, Derived>::AcquireConnection(
    engine::Deadline deadline) {
  EnsureInitialized();

  auto connection_ptr = Pop(deadline);
  return {this->shared_from_this(), std::move(connection_ptr)};
}

template <class Connection, class Derived>
void ConnectionPoolBase<Connection, Derived>::ReleaseConnection(
    ConnectionUniquePtr connection_ptr) {
  EnsureInitialized();
  UASSERT(connection_ptr);

  DoRelease(std::move(connection_ptr));

  given_away_semaphore_.unlock_shared();
  AsDerived().AccountConnectionReleased();
}

template <class Connection, class Derived>
Derived& ConnectionPoolBase<Connection, Derived>::AsDerived() noexcept {
  return *static_cast<Derived*>(this);
}

template <class Connection, class Derived>
const Derived& ConnectionPoolBase<Connection, Derived>::AsDerived() const
    noexcept {
  return *static_cast<const Derived*>(this);
}

template <class Connection, class Derived>
typename ConnectionPoolBase<Connection, Derived>::ConnectionUniquePtr
ConnectionPoolBase<Connection, Derived>::CreateConnection(
    const engine::SemaphoreLock& connecting_lock, engine::Deadline deadline) {
  EnsureInitialized();

  UASSERT(connecting_lock.OwnsLock());
  auto connection_ptr = AsDerived().DoCreateConnection(deadline);

  alive_connections_.fetch_add(1);
  AsDerived().AccountConnectionCreated();

  return connection_ptr;
}

template <class Connection, class Derived>
typename ConnectionPoolBase<Connection, Derived>::ConnectionUniquePtr
ConnectionPoolBase<Connection, Derived>::Pop(engine::Deadline deadline) {
  EnsureInitialized();

  engine::SemaphoreLock given_away_lock{given_away_semaphore_, deadline};
  if (!given_away_lock.OwnsLock()) {
    AsDerived().AccountOverload();
    throw PoolWaitLimitExceededError{};
  }

  auto connection_ptr = TryPop();
  if (!connection_ptr) {
    engine::SemaphoreLock connecting_lock{connecting_semaphore_, deadline};

    connection_ptr = TryPop();
    if (!connection_ptr) {
      if (!connecting_lock.OwnsLock()) {
        AsDerived().AccountOverload();
        throw PoolWaitLimitExceededError{};
      }
      connection_ptr = CreateConnection(connecting_lock, deadline);
    }
  }

  UASSERT(connection_ptr);

  given_away_lock.Release();
  AsDerived().AccountConnectionAcquired();

  return connection_ptr;
}

template <class Connection, class Derived>
typename ConnectionPoolBase<Connection, Derived>::ConnectionUniquePtr
ConnectionPoolBase<Connection, Derived>::TryPop() {
  EnsureInitialized();

  ConnectionRawPtr connection_ptr{nullptr};
  if (!queue_.pop(connection_ptr)) {
    return nullptr;
  }

  return ConnectionUniquePtr{connection_ptr};
}

template <class Connection, class Derived>
void ConnectionPoolBase<Connection, Derived>::DoRelease(
    ConnectionUniquePtr connection_ptr) {
  EnsureInitialized();
  UASSERT(connection_ptr);

  const auto is_broken = connection_ptr->IsBroken();
  ConnectionRawPtr connection_raw_ptr = connection_ptr.release();
  if (is_broken || !queue_.bounded_push(connection_raw_ptr)) {
    Drop(connection_raw_ptr);
  }
}

template <class Connection, class Derived>
void ConnectionPoolBase<Connection, Derived>::PushConnection(
    engine::Deadline deadline) {
  EnsureInitialized();

  engine::SemaphoreLock connecting_lock{connecting_semaphore_, deadline};
  if (!connecting_lock.OwnsLock()) {
    throw PoolWaitLimitExceededError{};
  }

  ConnectionUniquePtr connection_ptr =
      CreateConnection(connecting_lock, deadline);
  connecting_lock.Unlock();

  ConnectionRawPtr connection_raw_ptr = connection_ptr.release();
  if (!queue_.bounded_push(connection_raw_ptr)) {
    Drop(connection_raw_ptr);
  }
}

template <class Connection, class Derived>
void ConnectionPoolBase<Connection, Derived>::Drop(
    ConnectionRawPtr connection_ptr) noexcept {
  EnsureInitialized();
  std::default_delete<Connection>{}(connection_ptr);

  alive_connections_.fetch_sub(1);

  static_assert(noexcept(AsDerived().AccountConnectionDestroyed()),
                "Please make AccountConnectionDestroyed() noexcept, "
                "because it might get called in pool destructor and is "
                "expected to be noexcept");
  AsDerived().AccountConnectionDestroyed();
}

template <class Connection, class Derived>
std::size_t
ConnectionPoolBase<Connection, Derived>::AliveConnectionsCountApprox() const {
  EnsureInitialized();

  return alive_connections_.load();
}

template <class Connection, class Derived>
void ConnectionPoolBase<Connection, Derived>::NotifyConnectionWontBeReleased() {
  EnsureInitialized();

  given_away_semaphore_.unlock_shared();
  alive_connections_.fetch_sub(1);
}

template <class Connection, class Derived>
void ConnectionPoolBase<Connection, Derived>::CleanupQueue() {
  EnsureInitialized();

  ConnectionRawPtr connection_ptr{nullptr};

  while (queue_.pop(connection_ptr)) {
    Drop(connection_ptr);
  }
}

template <class Connection, class Derived>
void ConnectionPoolBase<Connection, Derived>::EnsureInitialized() const {
  UASSERT_MSG(
      initialized_,
      "Please call Init before invoking any other methods on connection pool.");
}

template <class Connection, class Derived>
void ConnectionPoolBase<Connection, Derived>::EnsureReset() const {
  UASSERT_MSG(reset_,
              "Please call Reset before base class is destroyed, otherwise no "
              "cleanup is performed.");
}

}  // namespace drivers::impl

USERVER_NAMESPACE_END
