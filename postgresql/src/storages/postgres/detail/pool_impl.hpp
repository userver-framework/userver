#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <boost/lockfree/queue.hpp>

#include <engine/condition_variable.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_with_result.hpp>
#include <error_injection/settings.hpp>
#include <rcu/rcu.hpp>
#include <utils/impl/wait_token_storage.hpp>
#include <utils/periodic_task.hpp>
#include <utils/token_bucket.hpp>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/detail/connection_ptr.hpp>
#include <storages/postgres/detail/non_transaction.hpp>
#include <storages/postgres/options.hpp>
#include <storages/postgres/statistics.hpp>
#include <storages/postgres/transaction.hpp>

namespace storages {
namespace postgres {
namespace detail {

class ConnectionPoolImpl
    : public std::enable_shared_from_this<ConnectionPoolImpl> {
 public:
  static std::shared_ptr<ConnectionPoolImpl> Create(
      const std::string& dsn, engine::TaskProcessor& bg_task_processor,
      PoolSettings pool_settings, ConnectionSettings conn_settings,
      CommandControl default_cmd_ctl,
      const error_injection::Settings& ei_settings);
  ~ConnectionPoolImpl();

  const std::string& GetDsn() const { return dsn_; }

  [[nodiscard]] ConnectionPtr Acquire(engine::Deadline);
  void Release(Connection* connection);

  size_t SizeApprox() const;
  const InstanceStatistics& GetStatistics() const;

  [[nodiscard]] Transaction Begin(const TransactionOptions& options,
                                  engine::Deadline deadline,
                                  OptionalCommandControl trx_cmd_ctl = {});

  [[nodiscard]] NonTransaction Start(engine::Deadline deadline);

  void SetDefaultCommandControl(CommandControl);

 protected:
  ConnectionPoolImpl(const std::string& dsn,
                     engine::TaskProcessor& bg_task_processor,
                     PoolSettings settings, ConnectionSettings conn_settings,
                     CommandControl default_cmd_ctl,
                     const error_injection::Settings& ei_settings);

 private:
  using ConnToken = ::utils::impl::WaitTokenStorage::Token;

  void Init();

  [[nodiscard]] engine::TaskWithResult<bool> Connect(ConnToken&&);

  void Push(Connection* connection);
  Connection* Pop(engine::Deadline);

  void Clear();

  void DeleteConnection(Connection* connection);
  void DeleteBrokenConnection(Connection* connection);

  void AccountConnectionStats(Connection::Statistics stats);

  Connection* AcquireImmediate();
  void MaintainConnections();
  void StartMaintainTask();
  void StopMaintainTask();

 private:
  using RecentCounter = ::utils::statistics::RecentPeriod<
      ::utils::statistics::RelaxedCounter<size_t>, size_t>;

  mutable InstanceStatistics stats_;
  std::string dsn_;
  PoolSettings settings_;
  ConnectionSettings conn_settings_;
  engine::TaskProcessor& bg_task_processor_;
  ::utils::PeriodicTask ping_task_;
  engine::Mutex wait_mutex_;
  engine::ConditionVariable conn_available_;
  boost::lockfree::queue<Connection*> queue_;
  ::utils::impl::WaitTokenStorage conn_token_storage_;
  std::atomic<size_t> wait_count_;
  rcu::Variable<CommandControl> default_cmd_ctl_;
  const error_injection::Settings ei_settings_;
  RecentCounter recent_conn_errors_;
  ::utils::TokenBucket cancel_limit_;
};

}  // namespace detail
}  // namespace postgres
}  // namespace storages
