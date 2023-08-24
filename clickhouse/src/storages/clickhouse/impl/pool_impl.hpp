#pragma once

#include <atomic>
#include <chrono>
#include <memory>

#include <boost/lockfree/queue.hpp>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/utils/datetime/steady_coarse_clock.hpp>
#include <userver/utils/periodic_task.hpp>

#include <userver/drivers/impl/connection_pool_base.hpp>

#include <storages/clickhouse/impl/settings.hpp>
#include <storages/clickhouse/stats/pool_statistics.hpp>
#include <storages/clickhouse/stats/statement_timer.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::impl {

class Connection;
class ConnectionPtr;

class PoolAvailabilityMonitor {
 public:
  using Clock = USERVER_NAMESPACE::utils::datetime::SteadyCoarseClock;
  using TimePoint = Clock::time_point;

  bool IsAvailable() const;

  void AccountSuccess() noexcept;
  void AccountFailure() noexcept;

 private:
  std::atomic<TimePoint> last_successful_communication_{TimePoint{}};
  std::atomic<TimePoint> last_unsuccessful_communication_{TimePoint{}};

  static_assert(std::atomic<TimePoint>::is_always_lock_free);
};

class PoolImpl final
    : public drivers::impl::ConnectionPoolBase<Connection, PoolImpl> {
 public:
  PoolImpl(clients::dns::Resolver&, PoolSettings&& settings);
  ~PoolImpl();

  bool IsAvailable() const;

  ConnectionPtr Acquire();
  void Release(Connection*);

  stats::PoolStatistics& GetStatistics() noexcept;

  const std::string& GetHostName() const;

  void StartMaintenance();

  stats::StatementTimer GetInsertTimer();
  stats::StatementTimer GetExecuteTimer();

 private:
  friend class drivers::impl::ConnectionPoolBase<Connection, PoolImpl>;

  void AccountConnectionAcquired();
  void AccountConnectionReleased();
  void AccountConnectionCreated();
  void AccountConnectionDestroyed() noexcept;
  void AccountOverload();

  ConnectionUniquePtr DoCreateConnection(engine::Deadline deadline);

  void StopMaintenance();
  void MaintainConnections();

  struct MaintenanceConnectionDeleter;

  clients::dns::Resolver& resolver_;
  const PoolSettings pool_settings_;

  stats::PoolStatistics statistics_{};

  PoolAvailabilityMonitor availability_monitor_{};
  USERVER_NAMESPACE::utils::PeriodicTask maintenance_task_;
};

}  // namespace storages::clickhouse::impl

USERVER_NAMESPACE_END
