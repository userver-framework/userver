#pragma once

#include <memory>
#include <string>

#include <userver/dynamic_config/source.hpp>

#include <storages/mongo/stats.hpp>
#include <userver/storages/mongo/pool_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl {

class PoolImpl {
 public:
  virtual ~PoolImpl() = default;

  const std::string& Id() const;
  const stats::PoolStatistics& GetStatistics() const;
  stats::PoolStatistics& GetStatistics();
  dynamic_config::Snapshot GetConfig() const;
  StatsVerbosity GetStatsVerbosity() const;

  virtual const std::string& DefaultDatabaseName() const = 0;

  virtual size_t InUseApprox() const = 0;
  virtual size_t SizeApprox() const = 0;
  virtual size_t MaxSize() const = 0;

 protected:
  PoolImpl(std::string&& id, const PoolConfig& static_config,
           dynamic_config::Source config_source);

 private:
  const std::string id_;
  const StatsVerbosity stats_verbosity_;
  const dynamic_config::Source config_source_;
  stats::PoolStatistics statistics_;
};

using PoolImplPtr = std::shared_ptr<PoolImpl>;

}  // namespace storages::mongo::impl

USERVER_NAMESPACE_END
