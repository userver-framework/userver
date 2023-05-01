#include <userver/server/websocket/websocket_server.h>
#include <userver/components/component.hpp>
#include <userver/concurrent/mpsc_queue.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/fast_scope_guard.hpp>

#include "utils.h"
#include "websocket_protocol.h"

namespace userver::websocket {

using utils::impl::Span;

static WebSocketServer::Config Parse(
    const userver::yaml_config::YamlConfig& value,
    userver::formats::parse::To<WebSocketServer::Config>) {
  WebSocketServer::Config config;
  config.max_remote_payload =
      value["max_remote_payload"].As<unsigned>(config.max_remote_payload);
  config.fragment_size =
      value["fragment_size"].As<unsigned>(config.fragment_size);
  config.debug_logging = value["debug_logging"].As<bool>(config.debug_logging);
  return config;
}

static Message CloseMessage(CloseStatusInt status) {
  return {{}, status, false, true};
}

static Message DataMessage(std::vector<std::byte>&& payload, bool is_text) {
  return {std::move(payload), {}, is_text, false};
}

class WebSocketConnectionImpl : public WebSocketConnection {
 private:
  std::unique_ptr<engine::io::RwBase> io;

  struct MessageExtended : Message {
    bool ping = false;
    bool pong = false;
  };

  using OutboxMessageQueue =
      userver::concurrent::MpscQueue<std::unique_ptr<MessageExtended>>;
  std::shared_ptr<InboxMessageQueue> inbox;
  std::shared_ptr<OutboxMessageQueue> outbox;
  OutboxMessageQueue::Producer outboxProducer;
  Headers headers;
  const engine::io::Sockaddr remoteAddr;

  WebSocketServer::Config config;

 public:
  WebSocketConnectionImpl(std::unique_ptr<engine::io::RwBase> io_, Headers&& h,
                          const engine::io::Sockaddr& remote_addr,
                          const WebSocketServer::Config& server_config)
      : io(std::move(io_)),
        inbox(InboxMessageQueue::Create(3)),
        outbox(OutboxMessageQueue::Create(3)),
        outboxProducer(outbox->GetMultiProducer()),
        headers(std::move(h)),
        remoteAddr(remote_addr),
        config(server_config) {}

  ~WebSocketConnectionImpl() {
    if (config.debug_logging) LOG_DEBUG() << "Websocket connection closed";
  }

  void ReadTaskCoro() {
    FrameParserState frame;
    InboxMessageQueue::Producer producer = inbox->GetProducer();
    try {
      while (!userver::engine::current_task::ShouldCancel()) {
        CloseStatusInt status =
            ReadWSFrame(frame, io.get(), config.max_remote_payload);
        if (config.debug_logging)
          LOG_DEBUG() << fmt::format(
              "Read frame isText {}, closed {}, data size {} status {} "
              "waitCont {}",
              frame.isText, frame.closed, frame.payload.size(), status,
              frame.waitingContinuation);
        if (status != 0) {
          MessageExtended closeMsg;
          closeMsg.remoteCloseStatus = status;
          SendExtended(std::move(closeMsg));
          std::ignore = producer.Push(CloseMessage(status));
          return;
        }
        if (frame.closed) {
          std::ignore = producer.Push(CloseMessage(frame.remoteCloseStatus));
          return;
        }
        if (frame.pingReceived) {
          MessageExtended pongMsg;
          pongMsg.pong = true;
          SendExtended(std::move(pongMsg));
          frame.pingReceived = false;
          continue;
        }
        if (frame.pongReceived) {
          frame.pongReceived = false;
          continue;
        }
        if (frame.waitingContinuation) continue;
        std::ignore = producer.Push(DataMessage(std::move(frame.payload), frame.isText));
      }
    } catch (std::exception const& e) {
      if (config.debug_logging)
        LOG_DEBUG() << "Exception during frame parsing " << e;
    }
    std::ignore = producer.Push(CloseMessage(static_cast<CloseStatusInt>(CloseStatus::kAbnormalClosure)));
  }

  void WriteTaskCoro() {
    OutboxMessageQueue::Consumer consumer = outbox->GetConsumer();
    while (!userver::engine::current_task::ShouldCancel()) {
      std::unique_ptr<MessageExtended> messagePtr;
      if (consumer.Pop(messagePtr)) {
        MessageExtended& message = *messagePtr;
        if (config.debug_logging)
          LOG_DEBUG() << "Write message " << message.data.size() << " bytes";
        if (message.ping) {
          SendExactly(io.get(), frames::PingFrame(), {});
        } else if (message.pong) {
          SendExactly(io.get(), frames::PongFrame(), {});
        } else if (message.remoteCloseStatus.has_value()) {
          SendExactly(io.get(),
                      frames::CloseFrame(message.remoteCloseStatus.value()),
                      {});
        } else if (!message.data.empty()) {
          Span<const std::byte> dataToSend{message.data};
          bool firstFrame = true;
          while (dataToSend.size() > config.fragment_size &&
                 config.fragment_size > 0) {
            SendExactly(
                io.get(),
                frames::DataFrame(dataToSend.first(config.fragment_size),
                                  message.isText, !firstFrame, false),
                {});
            firstFrame = false;
            dataToSend =
                dataToSend.last(dataToSend.size() - config.fragment_size);
          }
          SendExactly(
              io.get(),
              frames::DataFrame(dataToSend, message.isText, !firstFrame, true),
              {});
        }
      }
    }
  }

  bool SendExtended(MessageExtended&& message) {
    return outboxProducer.Push(
        std::make_unique<MessageExtended>(std::move(message)), {});
  }

  InboxMessageQueue::Consumer GetMessagesConsumer() override {
    return inbox->GetConsumer();
  }

  void Send(Message&& message) override {
    MessageExtended mext;
    mext.isText = message.isText;
    mext.remoteCloseStatus = message.remoteCloseStatus;
    mext.data = std::move(message.data);
    SendExtended(std::move(mext));
  }

  void Close(CloseStatusInt status_code) override {
    Send(CloseMessage(status_code));
  }

  const userver::engine::io::Sockaddr& RemoteAddr() const override {
    return remoteAddr;
  }

  const Headers& HandshakeHTTPHeaders() const override { return headers; }
};

void WebSocketServer::ProcessConnection(
    std::shared_ptr<WebSocketConnectionImpl> conn) {
  auto writeTask = userver::utils::Async(
      "ws-write", &WebSocketConnectionImpl::WriteTaskCoro, conn);
  onNewWSConnection(conn);
  conn->ReadTaskCoro();
}

WebSocketServer::WebSocketServer(
    const components::ComponentConfig& component_config,
    const components::ComponentContext& component_context)
    : HttpServer(component_config, component_context),
      config(component_config.As<Config>()) {}

Response WebSocketServer::HandleRequest(const Request& request) {
  if (!TestHeaderVal(request.headers, "Upgrade", "websocket") ||
      !TestHeaderVal(request.headers, "Connection", "Upgrade"))
    throw HttpStatusException(HttpStatus::kBadRequest);

  const std::string& secWebsocketKey =
      GetOptKey(request.headers, "Sec-WebSocket-Key", "");
  if (secWebsocketKey.empty())
    throw HttpStatusException(HttpStatus::kBadRequest);

  Response resp;
  resp.status = HttpStatus::kSwitchingProtocols;
  resp.headers["Connection"] = "Upgrade";
  resp.headers["Upgrade"] = "websocket";
  resp.headers["Sec-WebSocket-Accept"] = WebsocketSecAnswer(secWebsocketKey);
  resp.keepalive = true;

  resp.upgrade_connection =
      [this, headers = std::move(request.headers),
       remoteAddr = request.client_address](
          std::unique_ptr<engine::io::RwBase> io) mutable {
        this->ProcessConnection(std::make_shared<WebSocketConnectionImpl>(
            std::move(io), std::move(headers), *remoteAddr, this->config));
      };
  return resp;
}

}  // namespace userver::websocket
