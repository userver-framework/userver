#include <atomic>
#include <memory>
#include <string>
#include <string_view>

#include <fmt/format.h>
#include <boost/endian/conversion.hpp>

#include <curl-ev/native.hpp>
#include <userver/clients/http/client.hpp>
#include <userver/clients/http/websocket_response.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/simple_server.hpp>
#include <userver/utest/utest.hpp>
#include <userver/websocket/connection.hpp>
#include <userver/websocket/impl/protocol.hpp>

USERVER_NAMESPACE_BEGIN

#if defined(CURLWS_TEXT)

namespace {

using HttpResponse = utest::SimpleServer::Response;
using HttpRequest = utest::SimpleServer::Request;
using websocket::impl::frames::Continuation;
using websocket::impl::frames::Final;
using websocket::impl::frames::Masked;

constexpr std::string_view kPayload = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

std::string AppendHeaderAndPayload(
    boost::container::small_vector<char, websocket::impl::kMaxFrameHeaderSize> header,
    std::string_view payload
) {
    std::string frame;
    frame.append(header.data(), header.size());
    frame.append(payload);
    return frame;
}

std::string MakeUnmaskedDataFrame(
    std::string_view payload,
    bool is_text,
    Continuation is_continuation = Continuation::kNo,
    Final is_final = Final::kYes
) {
    return AppendHeaderAndPayload(
        websocket::impl::frames::DataFrameHeader(payload.size(), is_text, is_continuation, is_final, Masked::kNo),
        payload
    );
}

std::string MakeUnmaskedTextFrame(std::string_view payload) { return MakeUnmaskedDataFrame(payload, /*is_text=*/true); }

std::string MakeUnmaskedBinaryFrame(std::string_view payload) {
    return MakeUnmaskedDataFrame(payload, /*is_text=*/false);
}

std::string MakeUnmaskedControlFrame(websocket::impl::WSOpcodes opcode, std::string_view payload) {
    EXPECT_LE(payload.size(), 125);
    const auto header = websocket::impl::frames::MakeControlFrame(opcode, payload.size(), Masked::kNo);
    std::string frame;
    frame.append(header.data(), header.size());
    frame.append(payload);
    return frame;
}

std::string MakeUnmaskedCloseFrame(websocket::CloseStatus status) {
    const auto status_be = boost::endian::native_to_big(static_cast<websocket::CloseStatusInt>(status));
    std::string payload(reinterpret_cast<const char*>(&status_be), sizeof(status_be));
    return MakeUnmaskedControlFrame(websocket::impl::WSOpcodes::kClose, payload);
}

// Incomplete data frame: header advertises @a full_len, but only @a partial bytes follow.
std::string MakeIncompleteUnmaskedTextFrame(std::string_view partial, std::size_t full_len) {
    EXPECT_LT(partial.size(), full_len);
    return AppendHeaderAndPayload(
        websocket::impl::frames::DataFrameHeader(full_len, true, Continuation::kNo, Final::kYes, Masked::kNo),
        partial
    );
}

std::string ExtractHeaderValue(const HttpRequest& request, std::string_view name) {
    const auto needle = std::string(name) + ":";
    const auto pos = request.find(needle);
    if (pos == std::string::npos) {
        return {};
    }
    auto begin = pos + needle.size();
    while (begin < request.size() && (request[begin] == ' ' || request[begin] == '\t')) {
        ++begin;
    }
    const auto end = request.find("\r\n", begin);
    if (end == std::string::npos) {
        return {};
    }
    return request.substr(begin, end - begin);
}

HttpResponse MakeUpgradeResponse(const HttpRequest& request, std::string_view frames) {
    if (request.find("\r\n\r\n") == std::string::npos) {
        return {{}, HttpResponse::kTryReadMore};
    }

    const auto sec_key = ExtractHeaderValue(request, "Sec-WebSocket-Key");
    EXPECT_FALSE(sec_key.empty());

    std::string response =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: ";
    response += websocket::impl::WebsocketSecAnswer(sec_key);
    response += "\r\n\r\n";
    // Same TCP write as the 101 so libcurl CONNECT_ONLY buffers the frame(s).
    response += frames;

    return {std::move(response), HttpResponse::kWriteAndContinue};
}

HttpResponse WebSocketUpgradeWithImmediateMessage(const HttpRequest& request) {
    return MakeUpgradeResponse(request, MakeUnmaskedTextFrame(kPayload));
}

HttpResponse WebSocketUpgradeWithTwoImmediateMessages(const HttpRequest& request) {
    return MakeUpgradeResponse(request, MakeUnmaskedTextFrame("AAAA") + MakeUnmaskedTextFrame("BBBB"));
}

HttpResponse WebSocketUpgradeWithBinaryMessage(const HttpRequest& request) {
    return MakeUpgradeResponse(request, MakeUnmaskedBinaryFrame("bin-payload"));
}

HttpResponse WebSocketUpgradeWithPingThenText(const HttpRequest& request) {
    return MakeUpgradeResponse(
        request,
        MakeUnmaskedControlFrame(websocket::impl::WSOpcodes::kPing, "ping-body") + MakeUnmaskedTextFrame("after-ping")
    );
}

HttpResponse WebSocketUpgradeWithPongThenText(const HttpRequest& request) {
    // Empty pong payload: a non-empty body would stay in Message::data (unlike ping)
    // and make the following text frame look like a protocol error.
    return MakeUpgradeResponse(
        request,
        MakeUnmaskedControlFrame(websocket::impl::WSOpcodes::kPong, "") + MakeUnmaskedTextFrame("after-pong")
    );
}

HttpResponse WebSocketUpgradeWithClose(const HttpRequest& request) {
    return MakeUpgradeResponse(request, MakeUnmaskedCloseFrame(websocket::CloseStatus::kGoingAway));
}

HttpResponse WebSocketUpgradeWithFragmentedText(const HttpRequest& request) {
    // FIN=0 text + FIN=1 continuation -> one reassembled message.
    return MakeUpgradeResponse(
        request,
        MakeUnmaskedDataFrame("hel", true, Continuation::kNo, Final::kNo) +
            MakeUnmaskedDataFrame("lo!", true, Continuation::kYes, Final::kYes)
    );
}

HttpResponse WebSocketUpgradeWithFragmentedTextAndInterleavedPing(const HttpRequest& request) {
    // Control frames may appear between data fragments; drain must keep continuation state.
    return MakeUpgradeResponse(
        request,
        MakeUnmaskedDataFrame("AA", true, Continuation::kNo, Final::kNo) +
            MakeUnmaskedControlFrame(websocket::impl::WSOpcodes::kPing, "x") +
            MakeUnmaskedDataFrame("BB", true, Continuation::kYes, Final::kYes)
    );
}

HttpResponse WebSocketUpgradeWithLargeText(const HttpRequest& request) {
    // Larger than DrainCurlWebSocketPreamble scratch buffer (1024) so one frame
    // is assembled across multiple curl_ws_recv() calls.
    return MakeUpgradeResponse(request, MakeUnmaskedTextFrame(std::string(3000, 'L')));
}

bool TryRecvUntil(websocket::WebSocketConnection& conn, websocket::Message& msg, engine::Deadline deadline) {
    while (!deadline.IsReached()) {
        if (conn.TryRecv(msg)) {
            return true;
        }
        engine::Yield();
    }
    return false;
}

std::shared_ptr<websocket::WebSocketConnection> ConnectWs(
    const utest::SimpleServer& server,
    const websocket::Config& config = {}
) {
    auto http_client = utest::CreateHttpClient();
    auto ws_response =
        http_client->CreateRequest()
            .url(fmt::format("ws://127.0.0.1:{}", server.GetPort()))
            .timeout(utest::kMaxTestWaitTime)
            .PerformWebSocketHandshake();
    EXPECT_TRUE(ws_response.IsProtocolUpgraded());
    return ws_response.MakeWebSocketConnectionWithConfig(config);
}

}  // namespace

// libcurl CURLOPT_CONNECT_ONLY=2 can leave post-upgrade WebSocket bytes in its
// internal recv buffer. Without draining them before extracting the FD, the
// client loses those bytes. This is stable when 101 and the first frame arrive
// in one write (unlike sleeping after a normal server-side SendText).
UTEST(HttpClientWebSocket, HandshakePreambleDrained) {
    const utest::SimpleServer server{&WebSocketUpgradeWithImmediateMessage};

    auto conn = ConnectWs(server);

    websocket::Message msg;
    ASSERT_TRUE(TryRecvUntil(*conn, msg, engine::Deadline::FromDuration(utest::kMaxTestWaitTime))
    ) << "server message after handshake was lost (curl unread bytes not passed in?)";
    EXPECT_TRUE(msg.is_text);
    EXPECT_EQ(msg.data, kPayload);
    EXPECT_FALSE(msg.close_status.has_value());

    conn->Close(websocket::CloseStatus::kNormal);
}

// Same as HandshakePreambleDrained, but with two frames buffered after 101.
UTEST(HttpClientWebSocket, HandshakePreambleMultipleFramesDrained) {
    const utest::SimpleServer server{&WebSocketUpgradeWithTwoImmediateMessages};

    auto conn = ConnectWs(server);
    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

    websocket::Message msg;
    ASSERT_TRUE(TryRecvUntil(*conn, msg, deadline)) << "first buffered frame was lost";
    EXPECT_TRUE(msg.is_text);
    EXPECT_EQ(msg.data, "AAAA");

    ASSERT_TRUE(TryRecvUntil(*conn, msg, deadline)) << "second buffered frame was lost";
    EXPECT_TRUE(msg.is_text);
    EXPECT_EQ(msg.data, "BBBB");

    conn->Close(websocket::CloseStatus::kNormal);
}

UTEST(HttpClientWebSocket, HandshakePreambleBinaryDrained) {
    const utest::SimpleServer server{&WebSocketUpgradeWithBinaryMessage};

    auto conn = ConnectWs(server);

    websocket::Message msg;
    ASSERT_TRUE(TryRecvUntil(*conn, msg, engine::Deadline::FromDuration(utest::kMaxTestWaitTime)));
    EXPECT_FALSE(msg.is_text);
    EXPECT_EQ(msg.data, "bin-payload");

    conn->Close(websocket::CloseStatus::kNormal);
}

UTEST(HttpClientWebSocket, HandshakePreamblePingThenTextDrained) {
    const utest::SimpleServer server{&WebSocketUpgradeWithPingThenText};

    auto conn = ConnectWs(server);

    websocket::Message msg;
    ASSERT_TRUE(TryRecvUntil(*conn, msg, engine::Deadline::FromDuration(utest::kMaxTestWaitTime))
    ) << "text after drained ping was lost";
    EXPECT_TRUE(msg.is_text);
    EXPECT_EQ(msg.data, "after-ping");

    conn->Close(websocket::CloseStatus::kNormal);
}

UTEST(HttpClientWebSocket, HandshakePreamblePongThenTextDrained) {
    const utest::SimpleServer server{&WebSocketUpgradeWithPongThenText};

    auto conn = ConnectWs(server);

    websocket::Message msg;
    ASSERT_TRUE(TryRecvUntil(*conn, msg, engine::Deadline::FromDuration(utest::kMaxTestWaitTime))
    ) << "text after drained pong was lost";
    EXPECT_TRUE(msg.is_text);
    EXPECT_EQ(msg.data, "after-pong");

    conn->Close(websocket::CloseStatus::kNormal);
}

UTEST(HttpClientWebSocket, HandshakePreambleCloseDrained) {
    const utest::SimpleServer server{&WebSocketUpgradeWithClose};

    auto conn = ConnectWs(server);

    websocket::Message msg;
    ASSERT_TRUE(TryRecvUntil(*conn, msg, engine::Deadline::FromDuration(utest::kMaxTestWaitTime))
    ) << "close frame after handshake was lost";
    EXPECT_EQ(msg.close_status, websocket::CloseStatus::kGoingAway);
}

UTEST(HttpClientWebSocket, HandshakePreambleFragmentedTextDrained) {
    const utest::SimpleServer server{&WebSocketUpgradeWithFragmentedText};

    auto conn = ConnectWs(server);

    websocket::Message msg;
    ASSERT_TRUE(TryRecvUntil(*conn, msg, engine::Deadline::FromDuration(utest::kMaxTestWaitTime))
    ) << "reassembled fragmented message was lost";
    EXPECT_TRUE(msg.is_text);
    EXPECT_EQ(msg.data, "hello!");

    conn->Close(websocket::CloseStatus::kNormal);
}

UTEST(HttpClientWebSocket, HandshakePreambleFragmentedTextWithPingDrained) {
    const utest::SimpleServer server{&WebSocketUpgradeWithFragmentedTextAndInterleavedPing};

    auto conn = ConnectWs(server);

    websocket::Message msg;
    ASSERT_TRUE(TryRecvUntil(*conn, msg, engine::Deadline::FromDuration(utest::kMaxTestWaitTime))
    ) << "fragmented message with interleaved ping was lost";
    EXPECT_TRUE(msg.is_text);
    EXPECT_EQ(msg.data, "AABB");

    conn->Close(websocket::CloseStatus::kNormal);
}

UTEST(HttpClientWebSocket, HandshakePreambleLargeTextDrained) {
    const utest::SimpleServer server{&WebSocketUpgradeWithLargeText};

    auto conn = ConnectWs(server);

    websocket::Message msg;
    ASSERT_TRUE(TryRecvUntil(*conn, msg, engine::Deadline::FromDuration(utest::kMaxTestWaitTime))
    ) << "large multi-chunk drained frame was lost";
    EXPECT_TRUE(msg.is_text);
    EXPECT_EQ(msg.data, std::string(3000, 'L'));

    conn->Close(websocket::CloseStatus::kNormal);
}

// curl_ws_recv() returns a partial payload (bytesleft > 0) then CURLE_AGAIN:
// DrainCurlWebSocketPreamble rebuilds a full-length header + partial bytes;
// the unread tail must still be readable from the OS socket.
UTEST(HttpClientWebSocket, HandshakePreambleIncompleteFrameDrained) {
    constexpr std::string_view kPartial = "PARTIAL_";
    constexpr std::size_t kFullLen = 40;
    const std::string rest(kFullLen - kPartial.size(), 'R');

    const auto upgraded = std::make_shared<std::atomic<bool>>(false);
    const utest::SimpleServer server{[upgraded, rest, kPartial](const HttpRequest& request) -> HttpResponse {
        if (!upgraded->load()) {
            if (request.find("\r\n\r\n") == std::string::npos) {
                return {{}, HttpResponse::kTryReadMore};
            }
            upgraded->store(true);
            return MakeUpgradeResponse(request, MakeIncompleteUnmaskedTextFrame(kPartial, kFullLen));
        }
        // Client traffic (e.g. SendPing) triggers the second write with the payload tail.
        return {rest, HttpResponse::kWriteAndClose};
    }};

    auto conn = ConnectWs(server);
    // Wake the server so it can push the remaining payload bytes onto the socket.
    conn->SendPing();

    websocket::Message msg;
    ASSERT_TRUE(TryRecvUntil(*conn, msg, engine::Deadline::FromDuration(utest::kMaxTestWaitTime))
    ) << "incomplete drained frame + socket tail was not reassembled";
    EXPECT_TRUE(msg.is_text);
    EXPECT_EQ(msg.data, std::string(kPartial) + rest);

    conn->Close(websocket::CloseStatus::kNormal);
}

#endif  // defined(CURLWS_TEXT)

USERVER_NAMESPACE_END
