#pragma once

#include <memory>
#include <string>

#include <storages/mongo/stats.hpp>

namespace storages::mongo::impl {

class PoolImpl {
 public:
  virtual ~PoolImpl() = default;

  const std::string& Id() const;
  const stats::PoolStatistics& GetStatistics() const;
  stats::PoolStatistics& GetStatistics();

  virtual const std::string& DefaultDatabaseName() const = 0;

  virtual size_t InUseApprox() const = 0;
  virtual size_t SizeApprox() const = 0;
  virtual size_t MaxSize() const = 0;

 protected:
  explicit PoolImpl(std::string&& id);

 private:
  const std::string id_;
  stats::PoolStatistics statistics_;
};

using PoolImplPtr = std::shared_ptr<PoolImpl>;

}  // namespace storages::mongo::impl
