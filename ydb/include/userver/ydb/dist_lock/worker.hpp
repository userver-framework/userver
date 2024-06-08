#pragma once

/// @file userver/ydb/dist_lock/worker.hpp
/// @brief @copybrief ydb::DistLockedWorker

#include <atomic>
#include <functional>
#include <memory>
#include <string>

#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/not_null.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/ydb/coordination.hpp>
#include <userver/ydb/dist_lock/settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

namespace impl::dist_lock {
struct Statistics;
}  // namespace impl::dist_lock

/// A high-level primitive that perpetually tries to acquire a distributed lock
/// and runs user callback in a separate task while the lock is held.
/// Cancels the task when the lock is lost.
class DistLockedWorker final {
 public:
  using Callback = std::function<void()>;

  DistLockedWorker(engine::TaskProcessor& task_processor,
                   std::shared_ptr<CoordinationClient> coordination_client,
                   std::string_view coordination_node,
                   std::string_view semaphore_name, DistLockSettings settings,
                   Callback callback);

  ~DistLockedWorker();

  void Start();

  void Stop() noexcept;

  /// @brief Run the callback once under distlock, useful in tests.
  /// @warning Must not be run alongside a normal task launched via Start.
  void RunOnce();

  bool OwnsLock() const noexcept;

  friend void DumpMetric(utils::statistics::Writer& writer,
                         const DistLockedWorker& worker);

 private:
  void Run(bool run_once);

  engine::TaskProcessor& task_processor_;
  const std::shared_ptr<CoordinationClient> coordination_client_;
  const std::string coordination_node_;
  const std::string semaphore_name_;
  const DistLockSettings settings_;
  const Callback callback_;

  utils::UniqueRef<impl::dist_lock::Statistics> stats_;
  std::atomic<bool> owns_lock_{false};

  // Must be the last field, since it accesses other fields while alive.
  engine::TaskWithResult<void> worker_task_;
};

}  // namespace ydb

USERVER_NAMESPACE_END
