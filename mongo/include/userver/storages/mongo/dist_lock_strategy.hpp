#pragma once

#include <chrono>
#include <string>

#include <userver/dist_lock/dist_lock_strategy.hpp>
#include <userver/storages/mongo/collection.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

/// Strategy for mongodb-based distributed locking
class DistLockStrategy final : public dist_lock::DistLockStrategyBase {
 public:
  /// Targets a distributed lock in a specified collection as a host.
  DistLockStrategy(Collection collection, std::string lock_name);

  DistLockStrategy(Collection collection, std::string lock_name,
                   std::string owner);

  void Acquire(std::chrono::milliseconds lock_ttl,
               const std::string& locker_id) override;

  void Release(const std::string& locker_id) override;

 private:
  storages::mongo::Collection collection_;
  std::string lock_name_;
  std::string owner_prefix_;
};

}  // namespace storages::mongo

USERVER_NAMESPACE_END
