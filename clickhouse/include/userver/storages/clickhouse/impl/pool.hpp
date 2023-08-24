#pragma once

#include <memory>

#include <userver/storages/clickhouse/execution_result.hpp>
#include <userver/storages/clickhouse/options.hpp>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse {

class Query;

namespace impl {
class PoolImpl;
struct PoolSettings;
class InsertionRequest;

class Pool final {
 public:
  Pool(clients::dns::Resolver&, PoolSettings&&);
  ~Pool();

  Pool(const Pool&) = delete;
  Pool(Pool&&) = default;

  ExecutionResult Execute(OptionalCommandControl, const Query& query) const;

  void Insert(OptionalCommandControl, const InsertionRequest& request) const;

  void WriteStatistics(
      USERVER_NAMESPACE::utils::statistics::Writer& writer) const;

  bool IsAvailable() const;

 private:
  std::shared_ptr<PoolImpl> impl_;
};
}  // namespace impl

}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
