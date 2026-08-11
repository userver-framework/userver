#include <userver/clients/http/websocket_response.hpp>

#include <algorithm>
#include <cstring>
#include <string_view>

#include <curl-ev/native.hpp>
#include <userver/engine/io/prefixed_rw.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/websocket/connection.hpp>
#include <userver/websocket/impl/protocol.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {
namespace {

#if defined(CURLWS_TEXT)

// Rebuild a raw WebSocket frame from a curl_ws_recv() payload chunk.
// curl_ws_recv() strips the frame header and returns only decoded payload bytes
// (plus meta flags / bytesleft), so ReadWSFrame() cannot consume that buffer as-is:
// we must prepend a matching header via DataFrameHeader / MakeControlFrame.
// full_payload_len may be larger than payload.size() when the frame is incomplete
// (remainder still sits on the OS socket as raw payload bytes).
void AppendFrame(
    std::string& out,
    std::string_view payload,
    std::size_t full_payload_len,
    int flags,
    bool is_continuation
) {
    using websocket::impl::frames::Continuation;
    using websocket::impl::frames::Final;
    using websocket::impl::frames::Masked;

    if (flags & CURLWS_CLOSE) {
        const auto header = websocket::impl::frames::MakeControlFrame(
            websocket::impl::WSOpcodes::kClose,
            full_payload_len,
            Masked::kNo
        );
        out.append(header.data(), header.size());
        out.append(payload);
        return;
    }
    if (flags & CURLWS_PING) {
        const auto header =
            websocket::impl::frames::MakeControlFrame(websocket::impl::WSOpcodes::kPing, full_payload_len, Masked::kNo);
        out.append(header.data(), header.size());
        out.append(payload);
        return;
    }
    if (flags & CURLWS_PONG) {
        const auto header =
            websocket::impl::frames::MakeControlFrame(websocket::impl::WSOpcodes::kPong, full_payload_len, Masked::kNo);
        out.append(header.data(), header.size());
        out.append(payload);
        return;
    }

    const auto header = websocket::impl::frames::DataFrameHeader(
        full_payload_len,
        (flags & CURLWS_BINARY) == 0,
        is_continuation ? Continuation::kYes : Continuation::kNo,
        (flags & CURLWS_CONT) ? Final::kNo : Final::kYes,
        Masked::kNo
    );
    out.append(header.data(), header.size());
    out.append(payload);
}

// Drain post-upgrade WebSocket bytes buffered by libcurl CONNECT_ONLY=2.
//
// Why we cannot just return curl's buffer "as is" on libcurl 8.17:
// - During the 101 upgrade, any already-read WebSocket bytes after the HTTP
//   headers are stored in libcurl's internal ws->recvbuf, not left in the OS
//   socket receive queue. DupFd/extracted FD therefore misses them.
// - curl_easy_recv() does not surface ws->recvbuf on curl <= 8.17 (fixed later
//   via connection-level buffering, https://github.com/curl/curl/pull/22111 / cf_recvbuf). CURLWS_RAW_MODE
//   + curl_easy_recv was tried and fails HandshakePreambleDrained here.
// - curl_ws_recv() is the API that reads ws->recvbuf, but it returns decoded
//   payload only (headers already consumed). Feeding that straight into
//   ReadWSFrame() desyncs framing (e.g. payload byte 'X' misread as Close).
//
// So we curl_ws_recv() while data remains, then AppendFrame() to rebuild a raw
// frame stream for PrefixedRw / ReadWSFrame(). Incomplete frames use a
// header with the full length; the unread tail stays on the OS socket.
std::string DrainCurlWebSocketPreambleImpl(curl::native::CURL* easy) {
    UASSERT(easy);

    std::string preamble;
    std::string frame_payload;
    int frame_flags = 0;
    curl::native::curl_off_t last_bytes_left = 0;
    bool in_fragmented_message = false;
    bool have_partial_frame = false;

    // Small scratch buffer: curl_ws_recv may be called multiple times per frame.
    constexpr std::size_t kWsRecvChunkSize = 1024;

    while (true) {
        char buffer[kWsRecvChunkSize];
        size_t nread = 0;
        const curl::native::curl_ws_frame* meta = nullptr;
        const auto rc = curl::native::curl_ws_recv(easy, buffer, sizeof(buffer), &nread, &meta);

        if (rc == curl::native::CURLE_AGAIN) {
            break;
        }
        if (rc != curl::native::CURLE_OK) {
            LOG_DEBUG() << "curl_ws_recv while draining handshake preamble: " << curl::native::curl_easy_strerror(rc);
            break;
        }
        if (!meta) {
            break;
        }

        if (meta->offset == 0) {
            frame_payload.clear();
            frame_flags = meta->flags;
        }
        frame_payload.append(buffer, nread);
        last_bytes_left = meta->bytesleft;
        have_partial_frame = true;

        if (meta->bytesleft == 0) {
            AppendFrame(preamble, frame_payload, frame_payload.size(), frame_flags, in_fragmented_message);
            if ((frame_flags & (CURLWS_CLOSE | CURLWS_PING | CURLWS_PONG)) == 0) {
                in_fragmented_message = (frame_flags & CURLWS_CONT) != 0;
            }
            frame_payload.clear();
            have_partial_frame = false;
        }
    }

    if (have_partial_frame && !frame_payload.empty()) {
        // Header uses full length; remaining payload bytes stay on the OS socket.
        AppendFrame(
            preamble,
            frame_payload,
            frame_payload.size() + static_cast<std::size_t>(last_bytes_left),
            frame_flags,
            in_fragmented_message
        );
    }

    if (!preamble.empty()) {
        LOG_DEBUG() << "Drained " << preamble.size() << " bytes of WebSocket preamble from libcurl";
    }
    return preamble;
}

#else

// libcurl < 7.86 has no WebSocket API (curl_ws_recv / CURLWS_*).
std::string DrainCurlWebSocketPreambleImpl(curl::native::CURL*) { return {}; }

#endif

}  // namespace

WebSocketResponse::WebSocketResponse(
    std::shared_ptr<Response> handshake_response,
    fs::blocking::FileDescriptor&& socket,
    std::string socket_preamble
)
    : handshake_response_(std::move(handshake_response)),
      socket_(std::move(socket)),
      socket_preamble_(std::move(socket_preamble))
{}

bool WebSocketResponse::IsProtocolUpgraded() const {
    return handshake_response_ && handshake_response_->status_code() == Status::kSwitchingProtocols;
}

std::shared_ptr<websocket::WebSocketConnection> WebSocketResponse::MakeWebSocketConnection() {
    return MakeWebSocketConnectionWithConfig(websocket::Config{});
}

std::shared_ptr<websocket::WebSocketConnection> WebSocketResponse::MakeWebSocketConnectionWithConfig(
    const websocket::Config& config
) {
    if (!IsProtocolUpgraded()) {
        throw std::runtime_error("Protocol is not upgraded to WebSocket");
    }
    if (!socket_.IsOpen()) {
        throw std::runtime_error("WebSocketConnection has already been extracted");
    }

    auto socket = std::make_unique<engine::io::Socket>(socket_.GetNative());
    auto addr = socket->Getpeername();

    std::move(socket_).Release();

    std::unique_ptr<engine::io::RwBase> rw = std::move(socket);
    if (!socket_preamble_.empty()) {
        rw = std::make_unique<engine::io::PrefixedRw>(std::move(socket_preamble_), std::move(rw));
    }

    return websocket::MakeClientWebSocketConnection(std::move(rw), std::move(addr), config);
}

std::string impl::DrainCurlWebSocketPreamble(void* native_easy) {
    return DrainCurlWebSocketPreambleImpl(static_cast<curl::native::CURL*>(native_easy));
}

}  // namespace clients::http

USERVER_NAMESPACE_END
