#pragma once

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/dist_lock/dist_locked_worker.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/dist_lock_strategy.hpp>
#include <userver/utils/statistics/storage.hpp>

namespace storages::mongo {

class DistLockComponentBase : public components::LoggableComponentBase {
 public:
  DistLockComponentBase(const components::ComponentConfig&,
                        const components::ComponentContext&,
                        storages::mongo::Collection);

  ~DistLockComponentBase() override;

  dist_lock::DistLockedWorker& GetWorker();

 protected:
  /// Override this function with anything that must be done under the mongo
  /// lock
  virtual void DoWork() = 0;

  /// Must be called in ctr
  void Start();

  /// Must be called in dtr
  void Stop();

 private:
  std::unique_ptr<dist_lock::DistLockedWorker> worker_;
  ::utils::statistics::Entry statistics_holder_;
};

}  // namespace storages::mongo
