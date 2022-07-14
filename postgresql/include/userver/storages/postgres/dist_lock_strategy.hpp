#pragma once

/// @file userver/storages/postgres/dist_lock_strategy.hpp
/// @brief @copybrief storages::postgres::DistLockStrategy

#include <userver/dist_lock/dist_lock_settings.hpp>
#include <userver/dist_lock/dist_lock_strategy.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/storages/postgres/options.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

/// Postgres distributed locking strategy
class DistLockStrategy final : public dist_lock::DistLockStrategyBase {
 public:
  DistLockStrategy(ClusterPtr cluster, const std::string& table,
                   const std::string& lock_name,
                   const dist_lock::DistLockSettings& settings);

  void Acquire(std::chrono::milliseconds lock_ttl,
               const std::string& locker_id) override;

  void Release(const std::string& locker_id) override;

  void UpdateCommandControl(CommandControl cc);

 private:
  ClusterPtr cluster_;
  rcu::Variable<CommandControl> cc_;
  const std::string acquire_query_;
  const std::string release_query_;
  const std::string lock_name_;
  const std::string owner_prefix_;
};

}  // namespace storages::postgres

USERVER_NAMESPACE_END
