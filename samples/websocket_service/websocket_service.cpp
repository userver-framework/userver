#include <userver/utest/using_namespace_userver.hpp>

/// [Websocket service sample - component]
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/websocket/websocket_handler.hpp>
#include <userver/utils/daemon_run.hpp>

namespace samples::websocket {

class WebsocketsHandler final : public server::websocket::WebsocketHandlerBase {
 public:
  // `kName` is used as the component name in static config
  static constexpr std::string_view kName = "websocket-handler";

  // Component is valid after construction and is able to accept requests
  using WebsocketHandlerBase::WebsocketHandlerBase;

  void Handle(server::websocket::WebSocketConnection& chat,
              server::request::RequestContext&) const override {
    server::websocket::Message message;
    while (!engine::current_task::ShouldCancel()) {
      chat.Recv(message);
      if (message.close_status) break;
      chat.Send(std::move(message));
    }
    if (message.close_status) chat.Close(*message.close_status);
  }
};

}  // namespace samples::websocket
/// [Websocket service sample - component]

/// [Websocket service sample - main]
int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<samples::websocket::WebsocketsHandler>();
  return utils::DaemonMain(argc, argv, component_list);
}
/// [Websocket service sample - main]
