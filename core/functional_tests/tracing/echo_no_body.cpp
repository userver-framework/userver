#include <userver/utils/assert.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/http/url.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/utest/using_namespace_userver.hpp>

class EchoNoBody final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-echo-no-body";

  EchoNoBody(const components::ComponentConfig& config,
             const components::ComponentContext& context)
      : HttpHandlerBase(config, context),
        echo_url_{config["echo-url"].As<std::string>()},
        http_client_(
            context.FindComponent<components::HttpClient>().GetHttpClient()) {}

  std::string HandleRequestThrow(
      const server::http::HttpRequest&,
      server::request::RequestContext&) const override {
    auto response = http_client_.CreateRequest()
                        .get(echo_url_)
                        .retry(2)
                        .timeout(std::chrono::seconds{5})
                        .perform();
    response->raise_for_status();
    return {};
  }

  static yaml_config::Schema GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<server::handlers::HttpHandlerBase>(R"(
    type: object
    description: HTTP echo without body component
    additionalProperties: false
    properties:
        echo-url:
            type: string
            description: some other microservice listens on this URL
    )");
  }

 private:
  const std::string echo_url_;
  clients::http::Client& http_client_;
};

int main(int argc, char* argv[]) {
  const auto component_list = components::MinimalServerComponentList()
                                  .Append<EchoNoBody>()
                                  .Append<components::TestsuiteSupport>()
                                  .Append<server::handlers::TestsControl>()
                                  .Append<clients::dns::Component>()
                                  .Append<components::HttpClient>();
  return utils::DaemonMain(argc, argv, component_list);
}
