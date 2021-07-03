#pragma once

/// @file storages/postgres/dist_lock_component_base.hpp
/// @brief @copybrief storages::postgres::DistLockComponentBase

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/loggable_component_base.hpp>
#include <dist_lock/dist_locked_worker.hpp>
#include <storages/postgres/dist_lock_strategy.hpp>
#include <utils/statistics/storage.hpp>

namespace storages {
namespace postgres {

/// Base class for postgres-based distlock worker components
class DistLockComponentBase : public components::LoggableComponentBase {
 public:
  DistLockComponentBase(const components::ComponentConfig&,
                        const components::ComponentContext&);

  ~DistLockComponentBase();

  dist_lock::DistLockedWorker& GetWorker();

 protected:
  /// Override this function with anything that must be done under the pg lock
  virtual void DoWork() = 0;

  /// Must be called in ctr
  void AutostartDistLock();

  /// Must be called in dtr
  void StopDistLock();

 private:
  std::unique_ptr<dist_lock::DistLockedWorker> worker_;
  ::utils::statistics::Entry statistics_holder_;
  bool autostart_;
};

}  // namespace postgres
}  // namespace storages
