#pragma once

#include <atomic>

#include <userver/testsuite/tasks.hpp>
#include <userver/utils/periodic_task.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

namespace detail {
class ClusterImpl;
}

class ConnlimitWatchdog final {
 public:
  explicit ConnlimitWatchdog(detail::ClusterImpl& cluster,
                             testsuite::TestsuiteTasks& testsuite_tasks,
                             int shard_number,
                             std::function<void()> on_new_connlimit);

  void Start();

  void Stop();

  void Step();

  size_t GetConnlimit() const;

 private:
  detail::ClusterImpl& cluster_;
  std::atomic<size_t> connlimit_;
  std::function<void()> on_new_connlimit_;
  testsuite::TestsuiteTasks& testsuite_tasks_;
  int steps_with_errors_{0};
  USERVER_NAMESPACE::utils::PeriodicTask periodic_;
  int shard_number_;
};

}  // namespace storages::postgres

USERVER_NAMESPACE_END
