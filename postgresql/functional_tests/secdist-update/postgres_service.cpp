#include <userver/clients/dns/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/dynamic_config/client/component.hpp>
#include <userver/dynamic_config/updater/component.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/utils/from_string.hpp>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace chaos {

namespace {
const storages::postgres::Query kSelectMany{
    "SELECT generate_series(1, 100)",
    storages::postgres::Query::Name{"chaos_select_many"},
};
}

class PostgresHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-postgres";

    PostgresHandler(const components::ComponentConfig& config, const components::ComponentContext& context);

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const override;

private:
    storages::postgres::ClusterPtr pg_cluster_;
};

PostgresHandler::PostgresHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(context.FindComponent<components::Postgres>("key-value-database").GetCluster()) {}

std::string PostgresHandler::HandleRequestThrow(const server::http::HttpRequest&, server::request::RequestContext&)
    const {
    const std::chrono::milliseconds timeout{800};

    storages::postgres::CommandControl cc{timeout, timeout};
    auto transaction =
        pg_cluster_->Begin(storages::postgres::ClusterHostType::kMaster, storages::postgres::TransactionOptions{}, cc);

    TESTPOINT("pg-call", {});

    transaction.Execute(cc, kSelectMany);

    transaction.Commit();
    return "OK!";
}

}  // namespace chaos

int main(int argc, char* argv[]) {
    const auto component_list = components::MinimalServerComponentList()
                                    .Append<components::Secdist>()
                                    .Append<components::DefaultSecdistProvider>()
                                    .Append<chaos::PostgresHandler>()
                                    .Append<components::HttpClient>()
                                    .Append<components::Postgres>("key-value-database")
                                    .Append<components::TestsuiteSupport>()
                                    .Append<server::handlers::TestsControl>()
                                    .Append<clients::dns::Component>();
    return utils::DaemonMain(argc, argv, component_list);
}
