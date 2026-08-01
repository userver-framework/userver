#include <userver/websocket/impl/protocol.hpp>

#include <cstring>
#include <string>
#include <vector>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// Serves a prepared byte sequence to the frame parser.
class BufferReader final : public engine::io::ReadableBase {
public:
    explicit BufferReader(std::string data) : data_(std::move(data)) {}

    bool IsValid() const override { return true; }

    bool WaitReadable(engine::Deadline) override { return true; }

    size_t ReadSome(void* buf, size_t len, engine::Deadline deadline) override { return ReadAll(buf, len, deadline); }

    size_t ReadAll(void* buf, size_t len, engine::Deadline) override {
        const size_t left = data_.size() - pos_;
        const size_t count = std::min(len, left);
        std::memcpy(buf, data_.data() + pos_, count);
        pos_ += count;
        return count;
    }

private:
    std::string data_;
    size_t pos_{0};
};

constexpr unsigned kMaxPayloadSize = 65536;

// opcode 0x9 (ping) / 0xA (pong), FIN set, MASK set, 16-bit extended length
std::string MakeExtendedControlFrame(unsigned char opcode, size_t payload_size) {
    std::string frame;
    frame.push_back(static_cast<char>(0x80 | opcode));
    frame.push_back(static_cast<char>(0x80 | 126));
    frame.push_back(static_cast<char>((payload_size >> 8) & 0xff));
    frame.push_back(static_cast<char>(payload_size & 0xff));
    frame.append(4, '\0');  // masking key
    frame.append(payload_size, 'A');
    return frame;
}

std::string MakeShortPingFrame(size_t payload_size) {
    std::string frame;
    frame.push_back(static_cast<char>(0x80 | 0x9));
    frame.push_back(static_cast<char>(0x80 | payload_size));
    frame.append(4, '\0');  // masking key
    frame.append(payload_size, 'A');
    return frame;
}

}  // namespace

UTEST(WebsocketProtocol, PingWithExtendedLengthIsRejected) {
    BufferReader reader{MakeExtendedControlFrame(0x9, 200)};

    std::string payload;
    websocket::impl::FrameParserState frame;
    frame.payload = &payload;
    std::size_t payload_len = 0;

    EXPECT_EQ(
        websocket::impl::ReadWSFrame(frame, reader, kMaxPayloadSize, payload_len),
        websocket::CloseStatus::kProtocolError
    );
}

UTEST(WebsocketProtocol, PongWithExtendedLengthIsRejected) {
    BufferReader reader{MakeExtendedControlFrame(0xA, 200)};

    std::string payload;
    websocket::impl::FrameParserState frame;
    frame.payload = &payload;
    std::size_t payload_len = 0;

    EXPECT_EQ(
        websocket::impl::ReadWSFrame(frame, reader, kMaxPayloadSize, payload_len),
        websocket::CloseStatus::kProtocolError
    );
}

UTEST(WebsocketProtocol, PingWithinControlFrameLimitIsAccepted) {
    BufferReader reader{MakeShortPingFrame(125)};

    std::string payload;
    websocket::impl::FrameParserState frame;
    frame.payload = &payload;
    std::size_t payload_len = 0;

    EXPECT_EQ(
        websocket::impl::ReadWSFrame(frame, reader, kMaxPayloadSize, payload_len),
        websocket::CloseStatus::kNone
    );
    EXPECT_TRUE(frame.ping_received);
    EXPECT_EQ(payload_len, 125);
    EXPECT_EQ(payload, std::string(125, 'A'));
}

// A ping is allowed to arrive in the middle of a fragmented data message, so
// the buffer holds the unfinished data too. Only the ping's own bytes may be
// echoed back in the pong.
UTEST(WebsocketProtocol, PingDuringFragmentedMessageKeepsDataSeparate) {
    std::string data_frame;
    data_frame.push_back(static_cast<char>(0x1));  // text, FIN not set
    data_frame.push_back(static_cast<char>(0x80 | 100));
    data_frame.append(4, '\0');
    data_frame.append(100, 'D');

    BufferReader reader{data_frame + MakeShortPingFrame(100)};

    std::string payload;
    websocket::impl::FrameParserState frame;
    frame.payload = &payload;
    std::size_t payload_len = 0;

    ASSERT_EQ(
        websocket::impl::ReadWSFrame(frame, reader, kMaxPayloadSize, payload_len),
        websocket::CloseStatus::kNone
    );
    ASSERT_TRUE(frame.waiting_continuation);

    ASSERT_EQ(
        websocket::impl::ReadWSFrame(frame, reader, kMaxPayloadSize, payload_len),
        websocket::CloseStatus::kNone
    );
    EXPECT_TRUE(frame.ping_received);
    EXPECT_EQ(payload_len, 100);
    EXPECT_EQ(payload.size(), 200);
    // the pong payload is the tail of the buffer and stays within the limit
    EXPECT_EQ(payload.substr(payload.size() - payload_len), std::string(100, 'A'));
}

TEST(WebsocketProtocol, ControlFrameHeaderKeepsPayloadLength) {
    const std::string payload(125, 'A');
    const auto header = websocket::impl::frames::MakeControlFrame(
        websocket::impl::WSOpcodes::kPong,
        utils::as_bytes(utils::span<const char>{payload}),
        websocket::impl::frames::Masked::kNo
    );

    EXPECT_EQ(static_cast<unsigned char>(header[1]) & 0x7f, 125);
}

USERVER_NAMESPACE_END
