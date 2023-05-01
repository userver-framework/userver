#pragma once
#include <userver/concurrent/queue.hpp>
#include "http_server.h"

namespace userver::websocket {

using CloseStatusInt = int16_t;

enum class CloseStatus : CloseStatusInt {
  kNormal = 1000,
  kGoingAway = 1001,
  kProtocolError = 1002,
  kUnsupportedData = 1003,
  kFrameTooLarge = 1004,
  kNoStatusRcvd = 1005,
  kAbnormalClosure = 1006,
  kBadMessageData = 1007,
  kPolicyViolation = 1008,
  kTooBigData = 1009,
  kExtensionMismatch = 1010,
  kServerError = 1011
};

struct Message {
  std::vector<std::byte> data;
  std::optional<CloseStatusInt> remoteCloseStatus;
  bool isText = false;
  bool closed = false;
};

class WebSocketConnectionImpl;

using InboxMessageQueue = userver::concurrent::SpscQueue<Message>;

class WebSocketConnection {
 public:
  virtual InboxMessageQueue::Consumer GetMessagesConsumer() = 0;
  virtual void Send(Message&& message) = 0;
  virtual void Close(CloseStatusInt status_code) = 0;

  virtual const userver::engine::io::Sockaddr& RemoteAddr() const = 0;
  virtual const Headers& HandshakeHTTPHeaders() const = 0;
};

class WebSocketServer : public HttpServer {
 public:
  struct Config {
    bool debug_logging = false;
    unsigned max_remote_payload = 65536;
    unsigned fragment_size = 65536;  // 0 - do not fragment
  };

  WebSocketServer(
      const userver::components::ComponentConfig& component_config,
      const userver::components::ComponentContext& component_context);

 private:
  void ProcessConnection(std::shared_ptr<WebSocketConnectionImpl> conn);
  Response HandleRequest(const Request& request) final;

  virtual void onNewWSConnection(
      std::shared_ptr<WebSocketConnection> connection) = 0;

  Config config;
};

}  // namespace userver::websocket
