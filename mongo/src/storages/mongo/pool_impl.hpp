#pragma once

#include <memory>
#include <string>

#include <userver/dynamic_config/source.hpp>

#include <storages/mongo/congestion_control/limiter.hpp>
#include <storages/mongo/congestion_control/sensor.hpp>
#include <storages/mongo/stats.hpp>

#include <userver/congestion_control/controllers/linear.hpp>
#include <userver/storages/mongo/pool_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl {

class PoolImpl {
 public:
  virtual ~PoolImpl() = default;

  void Start();

  const std::string& Id() const;
  const stats::PoolStatistics& GetStatistics() const;
  stats::PoolStatistics& GetStatistics();
  dynamic_config::Snapshot GetConfig() const;
  StatsVerbosity GetStatsVerbosity() const;

  virtual const std::string& DefaultDatabaseName() const = 0;

  virtual void Ping() = 0;

  virtual size_t InUseApprox() const = 0;
  virtual size_t SizeApprox() const = 0;
  virtual size_t MaxSize() const = 0;
  virtual void SetMaxSize(size_t max_size) = 0;

 protected:
  PoolImpl(std::string&& id, const PoolConfig& static_config,
           dynamic_config::Source config_source);

 private:
  void OnConfigUpdate(const dynamic_config::Snapshot& config);

  const std::string id_;
  const StatsVerbosity stats_verbosity_;
  dynamic_config::Source config_source_;
  stats::PoolStatistics statistics_;

  // congestion control stuff
  cc::Sensor cc_sensor_;
  cc::Limiter cc_limiter_;
  congestion_control::v2::LinearController cc_controller_;

  // Must be the last field due to fields' RAII destruction order
  concurrent::AsyncEventSubscriberScope config_subscriber_;
};

using PoolImplPtr = std::shared_ptr<PoolImpl>;

}  // namespace storages::mongo::impl

USERVER_NAMESPACE_END
