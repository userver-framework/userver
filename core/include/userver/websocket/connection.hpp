#pragma once

/// @file userver/server/websocket/server.hpp
/// @brief @copybrief server::websocket::WebSocketConnection

#include <memory>

#include <userver/engine/io/socket.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/span.hpp>
#include <userver/websocket/message.hpp>
#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace websocket {

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

    /// Closes the connection by closing the underlying OS socket.
    virtual ~WebSocketConnection();

    /// @brief Read a message from websocket, handling pings under the hood.
    /// @param message input message
    /// @throws engine::io::IoException in case of socket errors
    /// @note Recv() is **not** thread-safe by itself (you may not call Recv() from
    /// multiple coroutines at once). It is **not** safe to call Recv() and Send() from different coroutines
    /// at once if TLS is used. Consider using Send()+TryRecv() from the same coroutine instead.
    virtual void Recv(Message& message) = 0;

    /// @brief Behaves in the same way as Recv(), but in case of first bytes of
    /// message are not yet ready to receive gives the control up to a client.
    /// @returns false in case of messages absence, otherwise true and behaves
    /// like Recv()
    virtual bool TryRecv(Message& message) = 0;

    /// @brief Send a message to websocket.
    /// @param message message to send
    /// @throws engine::io::IoException in case of socket errors
    /// @note Send() is not thread-safe by itself (you may not call Send() from
    /// multiple coroutines at once). It is **not** safe to call Recv() and Send() from different coroutines
    /// at once if TLS is used. Consider using Send()+TryRecv() from the same coroutine instead.
    virtual void Send(const Message& message) = 0;

    /// @copydoc Send
    virtual void SendText(std::string_view message) = 0;

    /// @brief Send a ping message to websocket.
    /// @throws engine::io::IoException in case of socket errors
    virtual void SendPing() = 0;

    /// @brief Get the number of not answered sequential pings;
    /// calls to SendPing() increment this value, Recv and TryRecv
    /// reset this value if some 'pong' is received.
    /// @returns the number of not answered sequential pings
    virtual std::size_t NotAnsweredSequentialPingsCount() = 0;

    /// @brief Sends binary data from container @b message.
    template <typename ContiguousContainer>
    void SendBinary(const ContiguousContainer& message) {
        static_assert(
            sizeof(typename ContiguousContainer::value_type) == 1,
            "SendBinary() should send either std::bytes or chars"
        );
        DoSendBinary(utils::span(
            reinterpret_cast<const std::byte*>(message.data()),
            reinterpret_cast<const std::byte*>(message.data() + message.size())
        ));
    }

    /// @brief Closes the connection with specified @b status_code.
    virtual void Close(CloseStatus status_code) = 0;

    virtual const engine::io::Sockaddr& RemoteAddr() const = 0;

    virtual void AddFinalTags(tracing::Span& span) const = 0;
    virtual void AddStatistics(Statistics& stats) const = 0;

    virtual engine::io::ReadAwaiter& ReadAwaiter() = 0;
    virtual engine::io::WriteAwaiter& WriteAwaiter() = 0;

protected:
    virtual void DoSendBinary(utils::span<const std::byte> message) = 0;
};

std::shared_ptr<WebSocketConnection> MakeServerWebSocketConnection(
    std::unique_ptr<engine::io::RwBase>&& socket,
    engine::io::Sockaddr&& peer_name,
    const Config& config
);

std::shared_ptr<WebSocketConnection> MakeClientWebSocketConnection(
    std::unique_ptr<engine::io::RwBase>&& socket,
    engine::io::Sockaddr&& peer_name,
    const Config& config
);

}  // namespace websocket

USERVER_NAMESPACE_END
