#include <userver/storages/clickhouse/cluster.hpp>

#include <userver/components/component_config.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/formats/common/merge.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/statistics/metadata.hpp>

#include <storages/clickhouse/impl/settings.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse {

namespace {

size_t WrappingIncrement(std::atomic<size_t>& value, size_t mod,
                         size_t inc = 1) {
  // we don't actually care about order being broken once in 2^64 iterations
  return value.fetch_add(inc) % mod;
}

}  // namespace

Cluster::Cluster(clients::dns::Resolver& resolver,
                 const impl::ClickhouseSettings& settings,
                 const components::ComponentConfig& config) {
  const auto& endpoints = settings.endpoints;
  const auto& auth_settings = settings.auth_settings;

  std::vector<engine::TaskWithResult<impl::Pool>> init_tasks;
  init_tasks.reserve(endpoints.size());
  for (const auto& endpoint : endpoints) {
    init_tasks.emplace_back(USERVER_NAMESPACE::utils::Async(
        fmt::format("create_pool_{}", endpoint.host),
        [&resolver, &endpoint, &auth_settings, &config]() {
          return impl::Pool{
              resolver, impl::PoolSettings{config, endpoint, auth_settings}};
        }));
  }

  pools_.reserve(init_tasks.size());
  for (auto& init_task : init_tasks) {
    pools_.emplace_back(init_task.Get());
  }
}

Cluster::~Cluster() = default;

ExecutionResult Cluster::DoExecute(OptionalCommandControl optional_cc,
                                   const Query& query) const {
  return GetPool().Execute(optional_cc, query);
}

void Cluster::DoInsert(OptionalCommandControl optional_cc,
                       const impl::InsertionRequest& request) const {
  GetPool().Insert(optional_cc, request);
}

const impl::Pool& Cluster::GetPool() const {
  const auto pools_count = pools_.size();
  const auto current_pool_ind =
      WrappingIncrement(current_pool_ind_, pools_count);

  for (size_t i = 0; i < pools_count; ++i) {
    const auto& pool = pools_[(current_pool_ind + i) % pools_count];
    if (pool.IsAvailable()) {
      WrappingIncrement(current_pool_ind_, pools_count, i);
      return pool;
    }
  }

  throw NoAvailablePoolError{"No available pools in cluster."};
}

void Cluster::WriteStatistics(
    USERVER_NAMESPACE::utils::statistics::Writer& writer) const {
  for (const auto& pool : pools_) {
    pool.WriteStatistics(writer);
  }
}
}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
