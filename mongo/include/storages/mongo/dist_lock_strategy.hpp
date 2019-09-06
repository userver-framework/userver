#pragma once

#include <chrono>
#include <string>

#include <dist_lock/dist_lock_strategy.hpp>
#include <storages/mongo/collection.hpp>

namespace storages::mongo {

/// Strategy for mongodb-based distributed locking
class DistLockStrategy final : public dist_lock::DistLockStrategyBase {
 public:
  /// Targets a distributed lock in a specified collection as a host.
  DistLockStrategy(Collection collection, std::string lock_name);

  DistLockStrategy(Collection collection, std::string lock_name,
                   std::string owner);

  void Acquire(std::chrono::milliseconds lock_ttl) override;

  void Release() override;

 private:
  storages::mongo::Collection collection_;
  std::string lock_name_;
  std::string owner_;
};

}  // namespace storages::mongo
