#pragma once

#include <storages/odbc/detail/topology/topology_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail::topology {

class Standalone final : public TopologyBase {
public:
    Standalone(
        const settings::ODBCClusterSettings& settings,
        const settings::StatementMetricsSettings& statement_metrics_settings,
        clients::dns::Resolver* resolver,
        engine::TaskProcessor& blocking_task_processor
    );
    ~Standalone() final;

private:
    Pool& GetPrimary() const final;
    Pool& GetSecondary() const final;

    Pool& InitializePoolReference() const;

    Pool& pool_;
};

}  // namespace storages::odbc::detail::topology

USERVER_NAMESPACE_END
