#include <userver/utest/using_namespace_userver.hpp>  // Note: this is for the purposes of samples only

#include <userver/easy.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/components/component_context.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

/// [ActionClient]
class ActionClient : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "action-client";

    ActionClient(const components::ComponentConfig& config, const components::ComponentContext& context)
        : ComponentBase{config, context},
          service_url_(config["service-url"].As<std::string>()),
          http_client_(context.FindComponent<components::HttpClient>().GetHttpClient()) {}

    auto CreateHttpRequest(std::string action) const {
        return http_client_.CreateRequest().url(service_url_).post().data(std::move(action)).perform();
    }

    static yaml_config::Schema GetStaticConfigSchema() {
        return yaml_config::MergeSchemas<components::ComponentBase>(R"(
    type: object
    description: My dependencies schema
    additionalProperties: false
    properties:
        service-url:
            type: string
            description: URL of the service to send the actions to
    )");
    }

private:
    const std::string service_url_;
    clients::http::Client& http_client_;
};
/// [ActionClient]

/// [ActionDep]
class ActionDep {
public:
    explicit ActionDep(const components::ComponentContext& config) : component_{config.FindComponent<ActionClient>()} {}
    auto CreateActionRequest(std::string action) const { return component_.CreateHttpRequest(std::move(action)); }

    static void RegisterOn(easy::HttpBase& app) {
        app.TryAddComponent<ActionClient>(ActionClient::kName, "service-url: http://some-service.example/v1/action");
        easy::HttpDep::RegisterOn(app);
    }

private:
    ActionClient& component_;
};

/// [main]
using Deps = easy::Dependencies<ActionDep, easy::PgDep>;
using DepsComponent = easy::DependenciesComponent<Deps>;

class MyHandler final : public server::handlers::HttpHandlerBase {
public:
    MyHandler(const components::ComponentConfig& config, const components::ComponentContext& component_context)
        : HttpHandlerBase(config, component_context),
          deps_(component_context.FindComponent<DepsComponent>().GetDependencies()) {}

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const override {
        const auto& action = request.GetArg("action");
        deps_.pg().Execute(
            storages::postgres::ClusterHostType::kMaster, "INSERT INTO events_table(action) VALUES($1)", action
        );
        return deps_.CreateActionRequest(action)->body();
    }

private:
    const Deps deps_;
};

int main(int argc, char* argv[]) {
    auto component_list = components::MinimalServerComponentList()
                              .Append<components::TestsuiteSupport>()
                              .Append<components::HttpClient>()
                              .Append<clients::dns::Component>()
                              .Append<ActionClient>()
                              .Append<DepsComponent>()
                              .Append<components::Postgres>("postgres")
                              .Append<MyHandler>("/log-POST");

    return utils::DaemonMain(argc, argv, component_list);
}
/// [main]
