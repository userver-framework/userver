#pragma once

#include <memory>
#include <string>

#include <userver/dynamic_config/source.hpp>

#include <storages/mongo/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl {

class PoolImpl {
 public:
  virtual ~PoolImpl() = default;

  const std::string& Id() const;
  const stats::PoolStatistics& GetStatistics() const;
  stats::PoolStatistics& GetStatistics();
  dynamic_config::Snapshot GetConfig() const;

  virtual const std::string& DefaultDatabaseName() const = 0;

  virtual size_t InUseApprox() const = 0;
  virtual size_t SizeApprox() const = 0;
  virtual size_t MaxSize() const = 0;

 protected:
  PoolImpl(std::string&& id, dynamic_config::Source config_source);

 private:
  const std::string id_;
  stats::PoolStatistics statistics_;
  dynamic_config::Source config_source_;
};

using PoolImplPtr = std::shared_ptr<PoolImpl>;

}  // namespace storages::mongo::impl

USERVER_NAMESPACE_END
