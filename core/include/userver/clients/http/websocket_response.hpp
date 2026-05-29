#pragma once

/// @file userver/clients/http/websocket_response.hpp
/// @brief @copybrief clients::http::WebSocketResponse

#include <userver/clients/http/response.hpp>
#include <userver/fs/blocking/file_descriptor.hpp>

USERVER_NAMESPACE_BEGIN

namespace websocket {
class WebSocketConnection;
}  // namespace websocket

namespace clients::http {

/// @brief HTTP response for WebSocket upgrade.
///
/// Call @ref http::client::Request::PerformWebSocketHandshake()
/// to get one. After successful WebSocket handshake, you can use
/// MakeWebSocketConnection() to establish a WebSocket connection.
///
/// @snippet samples/websocket_client/main.cpp WebSocket client sample - handler
class WebSocketResponse final {
public:
    /// @cond
    WebSocketResponse(std::shared_ptr<Response> handshake_response, fs::blocking::FileDescriptor&& socket);
    /// @endcond

    WebSocketResponse() = default;
    WebSocketResponse(WebSocketResponse&&) = default;
    WebSocketResponse(const WebSocketResponse&) = delete;
    WebSocketResponse& operator=(WebSocketResponse&&) = default;
    WebSocketResponse& operator=(const WebSocketResponse&) = delete;

    /// @brief Get the HTTP handshake response
    std::shared_ptr<Response> GetHandshakeResponse() { return handshake_response_; }

    /// @brief Check if WebSocket protocol upgrade was successful
    /// @returns true if handshake completed with status 101 Switching Protocols
    bool IsProtocolUpgraded() const;

    /// @brief Create a WebSocket connection from this response
    std::shared_ptr<websocket::WebSocketConnection> MakeWebSocketConnection();

private:
    std::shared_ptr<Response> handshake_response_;
    fs::blocking::FileDescriptor socket_;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
