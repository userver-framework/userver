#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

class HandlerHttp2 final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-http2";

  HandlerHttp2(const components::ComponentConfig& config,
               const components::ComponentContext& context)
      : server::handlers::HttpHandlerBase(config, context) {}

  std::string HandleRequestThrow(
      const server::http::HttpRequest& req,
      server::request::RequestContext&) const override {
    const auto& type = req.GetArg("type");
    if (type == "echo-body") {
      return req.RequestBody();
    } else if (type == "echo-header") {
      return req.GetHeader("echo-header");
    } else if (type == "sleep") {
      engine::SleepFor(std::chrono::seconds{1});
    } else if (type == "json") {
      formats::json::Value json = formats::json::FromString(req.RequestBody());
      return ToString(json);
    }
    return "";
  };
};

int main(int argc, char* argv[]) {
  auto component_list = components::MinimalServerComponentList()
                            .Append<components::TestsuiteSupport>()
                            .Append<components::HttpClient>()
                            .Append<clients::dns::Component>()
                            .Append<server::handlers::ServerMonitor>()
                            .Append<server::handlers::TestsControl>()
                            .Append<HandlerHttp2>()
                            .Append<server::handlers::Ping>();

  return utils::DaemonMain(argc, argv, component_list);
}
