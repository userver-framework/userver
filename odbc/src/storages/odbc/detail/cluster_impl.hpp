#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <userver/storages/odbc/cluster_types.hpp>
#include <userver/storages/odbc/query.hpp>
#include <userver/storages/odbc/result_set.hpp>
#include <userver/storages/odbc/settings.hpp>
#include <userver/storages/odbc/transaction.hpp>

#include <storages/odbc/detail/pool.hpp>
#include <storages/odbc/detail/topology/topology_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {
struct CommandControl;
}

namespace storages::odbc::detail {

class ClusterImpl {
public:
    ClusterImpl(const settings::ODBCClusterSettings& settings, clients::dns::Resolver* resolver);

    ~ClusterImpl() = default;

    ResultSet Execute(ClusterHostTypeFlags flags, const Query& query);

    ResultSet Execute(engine::Deadline deadline, ClusterHostTypeFlags flags, const Query& query);

    Transaction Begin(ClusterHostTypeFlags flags);

    Transaction Begin(engine::Deadline deadline, ClusterHostTypeFlags flags);

    void WriteStatistics(utils::statistics::Writer& writer) const;

    void SetDefaultCommandControl(const CommandControl& cc);

    std::optional<std::chrono::milliseconds> GetDefaultNetworkTimeout() const;

    std::optional<std::chrono::milliseconds> GetDefaultStatementTimeout() const;

private:
    Pool& SelectPool(ClusterHostTypeFlags flags) const;

    ResultSet ExecuteImpl(engine::Deadline effective_deadline, ClusterHostTypeFlags flags, const Query& query);

    Transaction BeginImpl(engine::Deadline effective_deadline, ClusterHostTypeFlags flags);

    std::unique_ptr<topology::TopologyBase> topology_;

    // Dynamic config: command control (timeouts)
    std::atomic<std::chrono::milliseconds> default_network_timeout_ms_{std::chrono::milliseconds::zero()};
    std::atomic<std::chrono::milliseconds> default_statement_timeout_ms_{std::chrono::milliseconds::zero()};
    std::atomic<bool> has_network_timeout_{false};
    std::atomic<bool> has_statement_timeout_{false};
};

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
