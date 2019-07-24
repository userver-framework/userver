#pragma once

#include <components/component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <storages/dist_locked_task.hpp>
#include <utils/statistics/storage.hpp>

#include <storages/postgres/dist_locked_task.hpp>

namespace storages {
namespace postgres {

class DistLockComponentBase : public components::LoggableComponentBase {
 public:
  DistLockComponentBase(const components::ComponentConfig&,
                        const components::ComponentContext&);

  ~DistLockComponentBase();

  DistLockedTask& GetTask();

 protected:
  /// Override this function with anything that must be done under the pg lock
  virtual void DoWork() = 0;

  /// Must be called in ctr
  void AutostartDistLock();

  /// Must be called in dtr
  void StopDistLock();

 private:
  formats::json::Value ExtendStatistics();

 private:
  ::utils::statistics::Entry statistics_holder_;
  std::unique_ptr<DistLockedTask> task_;
  bool autostart_;
};

}  // namespace postgres
}  // namespace storages
