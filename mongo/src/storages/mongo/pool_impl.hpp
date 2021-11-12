#pragma once

#include <memory>
#include <string>

#include <userver/rcu/rcu.hpp>

#include <storages/mongo/mongo_config.hpp>
#include <storages/mongo/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl {

class PoolImpl {
 public:
  virtual ~PoolImpl() = default;

  const std::string& Id() const;
  const stats::PoolStatistics& GetStatistics() const;
  stats::PoolStatistics& GetStatistics();
  rcu::ReadablePtr<Config> GetConfig() const;

  void SetConfig(Config config);

  virtual const std::string& DefaultDatabaseName() const = 0;

  virtual size_t InUseApprox() const = 0;
  virtual size_t SizeApprox() const = 0;
  virtual size_t MaxSize() const = 0;

 protected:
  PoolImpl(std::string&& id, Config config);

 private:
  const std::string id_;
  stats::PoolStatistics statistics_;
  rcu::Variable<Config> config_storage_;
};

using PoolImplPtr = std::shared_ptr<PoolImpl>;

}  // namespace storages::mongo::impl

USERVER_NAMESPACE_END
