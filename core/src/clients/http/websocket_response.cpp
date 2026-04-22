#include <userver/clients/http/websocket_response.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/websocket/connection.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

WebSocketResponse::WebSocketResponse(
    std::shared_ptr<Response> handshake_response,
    fs::blocking::FileDescriptor&& socket
)
    : handshake_response_(std::move(handshake_response)),
      socket_(std::move(socket))
{}

bool WebSocketResponse::IsProtocolUpgraded() const {
    return handshake_response_ && handshake_response_->status_code() == Status::kSwitchingProtocols;
}

std::shared_ptr<websocket::WebSocketConnection> WebSocketResponse::MakeWebSocketConnection() {
    if (!IsProtocolUpgraded()) {
        throw std::runtime_error("Protocol is not upgraded to WebSocket");
    }

    if (!socket_.IsOpen()) {
        throw std::runtime_error("WebSocketConnection has already been extracted");
    }

    auto socket = std::make_unique<engine::io::Socket>(socket_.GetNative());
    auto addr = socket->Getsockname();
    auto config = websocket::Config{};

    std::move(socket_).Release();

    return websocket::MakeClientWebSocketConnection(std::move(socket), std::move(addr), config);
}

}  // namespace clients::http

USERVER_NAMESPACE_END
