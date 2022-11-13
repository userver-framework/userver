#include <userver/clients/dns/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/utils/daemon_run.hpp>

namespace chaos {

const int kTimeoutSecs{10};

class HttpclientHandler final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-chaos-httpclient";

  HttpclientHandler(const components::ComponentConfig& config,
                    const components::ComponentContext& context);

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override;

 private:
  clients::http::Client& client_;
};

HttpclientHandler::HttpclientHandler(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      client_(context.FindComponent<components::HttpClient>().GetHttpClient()) {
}

std::string HttpclientHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&) const {
  const auto& type = request.GetArg("type");
  const auto& port = request.GetArg("port");
  const auto& timeout = request.GetArg("timeout");
  const std::chrono::seconds timeout_secs{timeout.empty() ? kTimeoutSecs
                                                          : std::stoi(timeout)};
  if (type == "common") {
    auto url = fmt::format("http://localhost:{}/test", port);
    auto response = client_.CreateNotSignedRequest()
                        ->get(url)
                        ->timeout(timeout_secs)
                        ->retry(1)
                        ->perform();
    response->raise_for_status();
    return response->body();
  }

  UASSERT(false);
  return {};
}

}  // namespace chaos

int main(int argc, char* argv[]) {
  const auto component_list = components::MinimalServerComponentList()
                                  .Append<chaos::HttpclientHandler>()
                                  .Append<components::HttpClient>()
                                  .Append<components::TestsuiteSupport>()
                                  .Append<server::handlers::TestsControl>()
                                  .Append<clients::dns::Component>();
  return utils::DaemonMain(argc, argv, component_list);
}
