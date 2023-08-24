#pragma once

#include <atomic>
#include <memory>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/utils/datetime/wall_coarse_clock.hpp>
#include <userver/utils/periodic_task.hpp>

#include <userver/drivers/impl/connection_pool_base.hpp>

#include <storages/mysql/infra/connection_ptr.hpp>
#include <storages/mysql/infra/statistics.hpp>
#include <storages/mysql/settings/settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace impl {
class Connection;
}

namespace infra {

class Pool final
    : public drivers::impl::ConnectionPoolBase<impl::Connection, Pool> {
 public:
  static std::shared_ptr<Pool> Create(
      clients::dns::Resolver& resolver,
      const settings::PoolSettings& pool_settings);
  ~Pool();

  ConnectionPtr Acquire(engine::Deadline deadline);
  void Release(ConnectionUniquePtr connection);

  void WriteStatistics(utils::statistics::Writer& writer) const;

  Pool(clients::dns::Resolver& resolver,
       const settings::PoolSettings& pool_settings);

 private:
  friend class drivers::impl::ConnectionPoolBase<impl::Connection, Pool>;

  ConnectionUniquePtr DoCreateConnection(engine::Deadline deadline);

  void AccountConnectionAcquired();
  void AccountConnectionReleased();
  void AccountConnectionCreated();
  void AccountConnectionDestroyed() noexcept;
  void AccountOverload();

  void RunSizeMonitor();
  void RunPinger();

  class PoolMonitor final {
   public:
    explicit PoolMonitor(Pool& pool);
    ~PoolMonitor();

    void Start();
    void Stop();

    void AccountSuccess() noexcept;
    void AccountFailure() noexcept;

    bool IsAvailable() noexcept;

   private:
    using Clock = utils::datetime::WallCoarseClock;
    static constexpr std::chrono::seconds kUnavailabilityThreshold{2};

    Pool& pool_;

    utils::PeriodicTask size_monitor_;
    utils::PeriodicTask pinger_;

    std::atomic<Clock::time_point> last_success_{Clock::time_point{}};
    std::atomic<Clock::time_point> last_failure_{Clock::time_point{}};
  };

  clients::dns::Resolver& resolver_;
  const settings::PoolSettings settings_;

  PoolConnectionStatistics stats_{};

  PoolMonitor monitor_;
};

}  // namespace infra

}  // namespace storages::mysql

USERVER_NAMESPACE_END
