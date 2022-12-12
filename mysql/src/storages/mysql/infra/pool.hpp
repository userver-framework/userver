#pragma once

#include <atomic>
#include <memory>

#include <boost/lockfree/queue.hpp>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/utils/datetime/wall_coarse_clock.hpp>
#include <userver/utils/periodic_task.hpp>

#include <storages/mysql/infra/connection_ptr.hpp>
#include <storages/mysql/settings/settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace impl {
class MySQLConnection;
}

namespace infra {

class Pool : public std::enable_shared_from_this<Pool> {
 public:
  static std::shared_ptr<Pool> Create(
      clients::dns::Resolver& resolver,
      const settings::PoolSettings& pool_settings);
  ~Pool();

  ConnectionPtr Acquire(engine::Deadline deadline);
  void Release(std::unique_ptr<impl::MySQLConnection> connection);

 protected:
  Pool(clients::dns::Resolver& resolver,
       const settings::PoolSettings& pool_settings);

 private:
  using RawConnectionPtr = impl::MySQLConnection*;
  using SmartConnectionPtr = std::unique_ptr<impl::MySQLConnection>;

  SmartConnectionPtr Pop(engine::Deadline deadline);
  SmartConnectionPtr TryPop();

  void DoRelease(SmartConnectionPtr connection);

  void PushConnection(engine::Deadline deadline);
  SmartConnectionPtr CreateConnection(engine::Deadline deadline);
  void Drop(RawConnectionPtr connection);

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

    std::atomic<Clock::time_point> last_success_{};
    std::atomic<Clock::time_point> last_failure_{};
  };

  clients::dns::Resolver& resolver_;
  const settings::PoolSettings settings_;

  engine::Semaphore given_away_semaphore_;
  engine::Semaphore connecting_semaphore_;
  boost::lockfree::queue<RawConnectionPtr> queue_;
  std::atomic<std::size_t> size_{0};

  PoolMonitor monitor_;
};

}  // namespace infra

}  // namespace storages::mysql

USERVER_NAMESPACE_END
