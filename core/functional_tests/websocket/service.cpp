#include <userver/clients/dns/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/sleep.hpp>
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
        const server::http::HttpRequest& request,
        server::http::HttpResponse&,
        server::request::RequestContext& context
    ) const override {
        context.SetUserData(HandshakeData{request.GetHeader("Origin")});
        return true;
    }

    void Handle(server::websocket::WebSocketConnection& chat, server::request::RequestContext& context) const override {
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

class WebsocketsHandlerAlt final : public server::websocket::WebsocketHandlerBase {
public:
    static constexpr std::string_view kName = "websocket-handler-alt";

    using WebsocketHandlerBase::WebsocketHandlerBase;

    void Handle(server::websocket::WebSocketConnection& chat, server::request::RequestContext&) const override {
        server::websocket::Message message;
        while (!engine::current_task::ShouldCancel()) {
            const bool msgIsReceived = chat.TryRecv(message);
            if (msgIsReceived) {
                if (message.close_status) break;
                chat.Send(std::move(message));
            } else {
                // we could've sent yet another server::websocket::Message
                // e.g. chat.SendBinary(server::websocket::Message{ "blah", {}, true });
            }
        }
        if (message.close_status) chat.Close(*message.close_status);
    }
};

class WebsocketsFullDuplexHandler final : public server::websocket::WebsocketHandlerBase {
public:
    static constexpr std::string_view kName = "websocket-duplex-handler";

    using WebsocketHandlerBase::WebsocketHandlerBase;

    void Handle(server::websocket::WebSocketConnection& chat, server::request::RequestContext&) const override {
        // Some sync data
        auto queue = concurrent::SpscQueue<std::string>::Create();

        auto reader = utils::Async("reader", [&chat, producer = queue->GetProducer()] {
            server::websocket::Message message;
            while (!engine::current_task::ShouldCancel()) {
                chat.Recv(message);
                if (message.close_status) break;
                [[maybe_unused]] auto ret = producer.Push(std::move(message.data));
            }
        });

        auto writer = utils::Async("writer", [&chat, consumer = queue->GetConsumer()] {
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

class WebsocketsPingPongHandler final : public server::websocket::WebsocketHandlerBase {
public:
    static constexpr std::string_view kName = "websocket-ping-pong-handler";

    using WebsocketHandlerBase::WebsocketHandlerBase;

    void Handle(server::websocket::WebSocketConnection& chat, server::request::RequestContext&) const override {
        std::chrono::milliseconds time_without_sends{0};
        while (!engine::current_task::ShouldCancel()) {
            if (chat.NotAnsweredSequentialPingsCount() > 3) {
                LOG_WARNING() << "Ping not answered, closing connection";
                chat.Close(server::websocket::CloseStatus::kGoingAway);
                break;
            }

            chat.SendPing();
            time_without_sends += std::chrono::milliseconds(200);
            engine::InterruptibleSleepFor(std::chrono::milliseconds(200));
        }
    }
};

int main(int argc, char* argv[]) {
    const auto component_list = components::MinimalServerComponentList()
                                    .Append<WebsocketsHandler>()
                                    .Append<WebsocketsHandlerAlt>()
                                    .Append<WebsocketsFullDuplexHandler>()
                                    .Append<WebsocketsPingPongHandler>()
                                    .Append<clients::dns::Component>()
                                    .Append<components::HttpClient>()
                                    .Append<components::TestsuiteSupport>()
                                    .Append<server::handlers::TestsControl>();
    return utils::DaemonMain(argc, argv, component_list);
}
