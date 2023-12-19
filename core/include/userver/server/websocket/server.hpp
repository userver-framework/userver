#pragma once

/// @file userver/server/websocket/server.hpp
/// @brief @copybrief websocket::WebSocketConnection

#include <memory>
#include <optional>

#include <userver/engine/io/socket.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/span.hpp>
#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::websocket {

using CloseStatusInt = int16_t;

/// @brief Close statuses
enum class CloseStatus : CloseStatusInt {
  kNone = 0,

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

/// @brief WebSocket message
struct Message {
  std::string data;                              ///< payload
  std::optional<CloseStatus> close_status = {};  ///< close status
  bool is_text = false;                          ///< is it text or binary?
};

class WebSocketConnectionImpl;

struct Config final {
  unsigned max_remote_payload = 65536;
  unsigned fragment_size = 65536;  // 0 - do not fragment
};

Config Parse(const yaml_config::YamlConfig&, formats::parse::To<Config>);

struct Statistics final {
  std::atomic<int64_t> msg_sent{0};
  std::atomic<int64_t> msg_recv{0};
  std::atomic<int64_t> bytes_sent{0};
  std::atomic<int64_t> bytes_recv{0};
};

/// @brief Main class for Websocket connection
class WebSocketConnection {
 public:
  WebSocketConnection();

  WebSocketConnection(WebSocketConnection&&) = delete;
  WebSocketConnection(const WebSocketConnection&) = delete;

  WebSocketConnection& operator=(WebSocketConnection&&) = delete;
  WebSocketConnection& operator=(const WebSocketConnection&) = delete;

  virtual ~WebSocketConnection();

  /// @brief Read a message from websocket, handling pings under the hood.
  /// @param message input message
  /// @throws engine::io::IoException in case of socket errors
  /// @note Recv() is not thread-safe by itself (you may not call Recv() from
  /// multiple coroutines at once), but it is safe to call Recv() and Send()
  /// from different coroutines at once thus implementing full-duplex socket
  /// connection.
  virtual void Recv(Message& message) = 0;

  /// @brief Send a message to websocket.
  /// @param message message to send
  /// @throws engine::io::IoException in case of socket errors
  /// @note Send() is not thread-safe by itself (you may not call Send() from
  /// multiple coroutines at once), but it is safe to call Recv() and Send()
  /// from different coroutines at once thus implementing full-duplex socket
  /// connection.
  virtual void Send(const Message& message) = 0;
  virtual void SendText(std::string_view message) = 0;

  template <typename ContiguousContainer>
  void SendBinary(const ContiguousContainer& message) {
    static_assert(sizeof(typename ContiguousContainer::value_type) == 1,
                  "SendBinary() should send either std::bytes or chars");
    DoSendBinary(utils::span(
        reinterpret_cast<const std::byte*>(message.data()),
        reinterpret_cast<const std::byte*>(message.data() + message.size())));
  }

  virtual void Close(CloseStatus status_code) = 0;

  virtual const engine::io::Sockaddr& RemoteAddr() const = 0;

  virtual void AddFinalTags(tracing::Span& span) const = 0;
  virtual void AddStatistics(Statistics& stats) const = 0;

 protected:
  virtual void DoSendBinary(utils::span<const std::byte> message) = 0;
};

std::shared_ptr<WebSocketConnection> MakeWebSocket(
    std::unique_ptr<engine::io::RwBase>&& socket,
    engine::io::Sockaddr&& peer_name, const Config& config);

}  // namespace server::websocket

USERVER_NAMESPACE_END
