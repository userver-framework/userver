#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/odbc.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

namespace odbc::secdist_update {

class Handler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName{"handler-odbc"};

    Handler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : server::handlers::HttpHandlerBase{config, context},
          // Deliberately cache the stable Cluster identity. A secdist update
          // must reconfigure this object in place.
          cluster_{context.FindComponent<components::Odbc>("odbc-database").GetCluster()}
    {}

    std::string HandleRequestThrow(const server::http::HttpRequest&, server::request::RequestContext&) const override {
        const auto result = cluster_->Execute(storages::odbc::ClusterHostType::kMaster, "SELECT ?::integer", 42);
        return std::to_string(result[0][0].GetInt32());
    }

private:
    const std::shared_ptr<storages::odbc::Cluster> cluster_;
};

}  // namespace odbc::secdist_update

int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .AppendComponentList(clients::http::ComponentList())
            .Append<odbc::secdist_update::Handler>()
            .Append<server::handlers::TestsControl>()
            .Append<components::TestsuiteSupport>()
            .Append<components::Secdist>()
            .Append<components::DefaultSecdistProvider>()
            .Append<clients::dns::Component>()
            .Append<components::Odbc>("odbc-database");
    return utils::DaemonMain(argc, argv, component_list);
}
