#pragma once

#include <atomic>
#include <memory>

#include <boost/lockfree/queue.hpp>

#include <userver/engine/deadline.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/utils/periodic_task.hpp>

#include <storages/mysql/infra/connection_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace impl {

class MySQLConnection;

}

namespace infra {

class Pool : public std::enable_shared_from_this<Pool> {
 public:
  static std::shared_ptr<Pool> Create();
  ~Pool();

  ConnectionPtr Acquire(engine::Deadline deadline);
  void Release(std::unique_ptr<impl::MySQLConnection> connection);

 protected:
  Pool();

 private:
  using RawConnectionPtr = impl::MySQLConnection*;
  using SmartConnectionPtr = std::unique_ptr<impl::MySQLConnection>;

  SmartConnectionPtr Pop(engine::Deadline deadline);
  SmartConnectionPtr TryPop();

  void PushConnection(engine::Deadline deadline);
  SmartConnectionPtr CreateConnection(engine::Deadline deadline);
  void Drop(RawConnectionPtr connection);

  void RunMonitor();

  engine::Semaphore given_away_semaphore_;
  engine::Semaphore connecting_semaphore_;
  boost::lockfree::queue<RawConnectionPtr> queue_;
  std::atomic<std::size_t> size_{0};

  utils::PeriodicTask monitor_;
};

}  // namespace infra

}  // namespace storages::mysql

USERVER_NAMESPACE_END
