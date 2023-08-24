#include <storages/postgres/detail/topology/base.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail::topology {
namespace {

// Special connection ID to ease detection in logs
constexpr uint32_t kConnectionId = 4'100'200'300;

}  // namespace

TopologyBase::TopologyBase(engine::TaskProcessor& bg_task_processor,
                           DsnList dsns, clients::dns::Resolver* resolver,
                           const TopologySettings& topology_settings,
                           const ConnectionSettings& conn_settings,
                           const DefaultCommandControls& default_cmd_ctls,
                           const testsuite::PostgresControl& testsuite_pg_ctl,
                           error_injection::Settings ei_settings)
    : bg_task_processor_(bg_task_processor),
      dsns_(std::move(dsns)),
      resolver_{resolver},
      topology_settings_(topology_settings),
      conn_settings_(conn_settings),
      default_cmd_ctls_(default_cmd_ctls),
      testsuite_pg_ctl_(testsuite_pg_ctl),
      ei_settings_(ei_settings) {}

const DsnList& TopologyBase::GetDsnList() const { return dsns_; }

const TopologySettings& TopologyBase::GetTopologySettings() const {
  return topology_settings_;
}

const testsuite::PostgresControl& TopologyBase::GetTestsuiteControl() const {
  return testsuite_pg_ctl_;
}

std::unique_ptr<Connection> TopologyBase::MakeTopologyConnection(DsnIndex idx) {
  UASSERT(idx < dsns_.size());
  return Connection::Connect(dsns_[idx], resolver_, bg_task_processor_,
                             bg_task_storage_, kConnectionId, conn_settings_,
                             default_cmd_ctls_, testsuite_pg_ctl_,
                             ei_settings_);
}

}  // namespace storages::postgres::detail::topology

USERVER_NAMESPACE_END
