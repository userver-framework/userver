#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/error_injection/settings.hpp>
#include <userver/testsuite/postgres_control.hpp>

#include <storages/postgres/detail/pg_impl_types.hpp>
#include <storages/postgres/detail/pool.hpp>
#include <storages/postgres/detail/statement_timings_storage.hpp>
#include <storages/postgres/detail/topology/base.hpp>
#include <userver/storages/postgres/cluster_types.hpp>
#include <userver/storages/postgres/detail/non_transaction.hpp>
#include <userver/storages/postgres/options.hpp>
#include <userver/storages/postgres/statistics.hpp>
#include <userver/storages/postgres/transaction.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

class ClusterImpl {
 public:
  ClusterImpl(DsnList dsns, clients::dns::Resolver* resolver,
              engine::TaskProcessor& bg_task_processor,
              const ClusterSettings& cluster_settings,
              const DefaultCommandControls& default_cmd_ctls,
              const testsuite::PostgresControl& testsuite_pg_ctl,
              const error_injection::Settings& ei_settings);
  ~ClusterImpl();

  ClusterStatisticsPtr GetStatistics() const;

  Transaction Begin(ClusterHostTypeFlags, const TransactionOptions&,
                    OptionalCommandControl);

  NonTransaction Start(ClusterHostTypeFlags, OptionalCommandControl);

  void SetDefaultCommandControl(CommandControl, DefaultCommandControlSource);
  CommandControl GetDefaultCommandControl() const;

  void SetHandlersCommandControl(
      const CommandControlByHandlerMap& handlers_command_control);

  void SetQueriesCommandControl(
      const CommandControlByQueryMap& queries_command_control);

  void SetConnectionSettings(const ConnectionSettings& settings);

  void SetPoolSettings(const PoolSettings& settings);

  void SetStatementMetricsSettings(const StatementMetricsSettings& settings);

  OptionalCommandControl GetQueryCmdCtl(const std::string& query_name) const;

  OptionalCommandControl GetTaskDataHandlersCommandControl() const;

 private:
  using ConnectionPoolPtr = std::shared_ptr<ConnectionPool>;

  ConnectionPoolPtr FindPool(ClusterHostTypeFlags);

  DefaultCommandControls default_cmd_ctls_;
  std::unique_ptr<topology::TopologyBase> topology_;
  engine::TaskProcessor& bg_task_processor_;
  std::vector<ConnectionPoolPtr> host_pools_;
  std::atomic<uint32_t> rr_host_idx_;
};

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
