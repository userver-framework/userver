#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/clients/http/websocket_response.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/websocket/connection.hpp>

namespace samples::websocket_client {

/// [WebSocket client sample - handler]
class WebSocketClientHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-websocket-client";

    WebSocketClientHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          http_client_(context.FindComponent<components::HttpClient>().GetHttpClient())
    {}

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const override {
        const auto& ws_server_url = request.GetArg("url");

        // Perform WebSocket handshake
        auto ws_response = http_client_.CreateRequest().url(ws_server_url).PerformWebSocketHandshake();

        // Create WebSocket connection
        auto conn = ws_response.MakeWebSocketConnection();

        conn->SendText(request.GetArg("message"));

        // Receive response
        websocket::Message response;
        conn->Recv(response);

        // Close connection
        conn->Close(websocket::CloseStatus::kNormal);

        return response.data;
    }

private:
    clients::http::Client& http_client_;
};
/// [WebSocket client sample - handler]

}  // namespace samples::websocket_client

/// [WebSocket client sample - main]
int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .Append<components::TestsuiteSupport>()
            .Append<server::handlers::TestsControl>()
            .Append<clients::dns::Component>()
            .AppendComponentList(clients::http::ComponentList())
            .Append<samples::websocket_client::WebSocketClientHandler>();
    return utils::DaemonMain(argc, argv, component_list);
}
/// [WebSocket client sample - main]
