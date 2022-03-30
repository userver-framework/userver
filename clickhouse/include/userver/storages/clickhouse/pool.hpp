#pragma once

#include <memory>

#include <userver/storages/clickhouse/execution_result.hpp>
#include <userver/storages/clickhouse/options.hpp>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse {

class Query;
class InsertionRequest;
struct PoolSettings;

namespace impl {
class PoolImpl;
}

class Pool final {
 public:
  Pool(clients::dns::Resolver*, PoolSettings&&);
  ~Pool();

  Pool(const Pool&) = delete;
  Pool(Pool&&) = default;

  ExecutionResult Execute(OptionalCommandControl, const Query& query) const;

  void Insert(OptionalCommandControl, const InsertionRequest& request) const;

  formats::json::Value GetStatistics() const;

 private:
  std::shared_ptr<impl::PoolImpl> impl_;
};

}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
