#include <userver/clients/dns/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/server/websocket/websocket_handler.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/utest/using_namespace_userver.hpp>

struct HandshakeData {
  std::string origin;
};

class WebsocketsHandler final : public server::websocket::WebsocketHandlerBase {
 public:
  static constexpr std::string_view kName = "websocket-handler";

  using WebsocketHandlerBase::WebsocketHandlerBase;

  bool HandleHandshake(
      const server::http::HttpRequest& request, server::http::HttpResponse&,
      server::request::RequestContext& context) const override {
    context.SetUserData(HandshakeData{request.GetHeader("Origin")});
    return true;
  }

  void Handle(server::websocket::WebSocketConnection& chat,
              server::request::RequestContext& context) const override {
    const auto& origin = context.GetUserData<HandshakeData>().origin;
    if (!origin.empty()) {
      chat.Send({origin, {}, true});
    }

    server::websocket::Message message;
    while (!engine::current_task::ShouldCancel()) {
      chat.Recv(message);

      if (message.close_status) break;

      if (message.data == "close") {
        chat.Close(server::websocket::CloseStatus::kGoingAway);
        break;
      }

      chat.Send(std::move(message));
    }
    if (message.close_status) chat.Close(*message.close_status);
  }
};

class WebsocketsFullDuplexHandler final
    : public server::websocket::WebsocketHandlerBase {
 public:
  static constexpr std::string_view kName = "websocket-duplex-handler";

  using WebsocketHandlerBase::WebsocketHandlerBase;

  void Handle(server::websocket::WebSocketConnection& chat,
              server::request::RequestContext&) const override {
    // Some sync data
    auto queue = concurrent::SpscQueue<std::string>::Create();

    auto reader =
        utils::Async("reader", [&chat, producer = queue->GetProducer()] {
          server::websocket::Message message;
          while (!engine::current_task::ShouldCancel()) {
            chat.Recv(message);
            if (message.close_status) break;
            [[maybe_unused]] auto ret = producer.Push(std::move(message.data));
          }
        });

    auto writer =
        utils::Async("writer", [&chat, consumer = queue->GetConsumer()] {
          while (!engine::current_task::ShouldCancel()) {
            std::string msg;
            if (!consumer.Pop(msg)) break;
            chat.SendBinary(msg);
          }
        });

    queue.reset();

    reader.Get();
    writer.Get();
  }
};

int main(int argc, char* argv[]) {
  const auto component_list = components::MinimalServerComponentList()
                                  .Append<WebsocketsHandler>()
                                  .Append<WebsocketsFullDuplexHandler>()
                                  .Append<clients::dns::Component>()
                                  .Append<components::HttpClient>()
                                  .Append<components::TestsuiteSupport>()
                                  .Append<server::handlers::TestsControl>();
  return utils::DaemonMain(argc, argv, component_list);
}
