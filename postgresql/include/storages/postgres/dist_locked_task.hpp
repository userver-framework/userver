#pragma once

#include <rcu/rcu.hpp>
#include <storages/dist_locked_task.hpp>
#include <storages/postgres/options.hpp>

namespace storages {
namespace postgres {

class DistLockedTask : public storages::DistLockedTask {
 public:
  DistLockedTask(ClusterPtr cluster, const std::string& table,
                 const std::string& lock_name, WorkerFunc worker_func,
                 const DistLockedTaskSettings& settings,
                 Mode mode = Mode::kLoop);

  void UpdateCommandControl(CommandControl cc);

 protected:
  void RequestAcquire(std::chrono::milliseconds lock_time) override;

  void RequestRelease() override;

 private:
  ClusterPtr cluster_;
  rcu::Variable<CommandControl> cc_;
  const std::string acquire_query_;
  const std::string release_query_;
  const std::string lock_name_;
  const std::string owner_;
};

}  // namespace postgres
}  // namespace storages
