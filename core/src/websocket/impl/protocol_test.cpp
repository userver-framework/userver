#include <userver/websocket/impl/protocol.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <boost/endian/conversion.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using websocket::impl::FrameParserState;
using websocket::impl::ReadWSFrame;
using websocket::impl::WSHeader;
using websocket::impl::WSOpcodes;
using websocket::impl::frames::Continuation;
using websocket::impl::frames::DataFrameHeader;
using websocket::impl::frames::Final;
using websocket::impl::frames::MakeControlFrame;
using websocket::impl::frames::Mask32;
using websocket::impl::frames::Masked;

constexpr char kPayloadFill = 'A';
constexpr char kDataFill = 'D';
constexpr std::size_t kMaxControlPayload = 125;
constexpr std::size_t kOversizedControlPayload = 200;
constexpr std::size_t kInterleavedFragmentSize = 100;
constexpr unsigned kMaxPayloadSize = 65536;
constexpr std::size_t kMaskingKeySize = sizeof(Mask32);
// RFC 6455: payload_len == 126 means a 16-bit extended length follows.
constexpr unsigned char kExtendedPayloadLen16 = 126;

// Serves a prepared byte sequence to the frame parser.
class BufferReader final : public engine::io::ReadableBase {
public:
    explicit BufferReader(std::string data)
        : data_(std::move(data))
    {}

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

std::string AppendMaskedPayload(std::string frame, std::size_t payload_size, char fill) {
    frame.append(kMaskingKeySize, '\0');
    frame.append(payload_size, fill);
    return frame;
}

// MakeControlFrame refuses payload_len > 125, so an invalid extended control
// frame is built via WSHeader + 16-bit length.
std::string MakeExtendedControlFrame(WSOpcodes opcode, size_t payload_size) {
    WSHeader hdr{};
    hdr.bits.fin = 1;
    hdr.bits.opcode = opcode;
    hdr.bits.mask = 1;
    hdr.bits.payload_len = kExtendedPayloadLen16;

    const auto len_be = boost::endian::native_to_big(static_cast<std::uint16_t>(payload_size));

    std::string frame;
    frame.append(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    frame.append(reinterpret_cast<const char*>(&len_be), sizeof(len_be));
    return AppendMaskedPayload(std::move(frame), payload_size, kPayloadFill);
}

std::string MakeShortPingFrame(size_t payload_size) {
    const auto header = MakeControlFrame(WSOpcodes::kPing, payload_size, Masked::kYes);
    std::string frame;
    frame.append(header.data(), header.size());
    return AppendMaskedPayload(std::move(frame), payload_size, kPayloadFill);
}

std::string MakeMaskedTextFragment(std::size_t payload_size, Final is_final) {
    const auto header = DataFrameHeader(
        payload_size,
        /*is_text=*/true,
        Continuation::kNo,
        is_final,
        Masked::kYes
    );
    std::string frame;
    frame.append(header.data(), header.size());
    return AppendMaskedPayload(std::move(frame), payload_size, kDataFill);
}

}  // namespace

UTEST(WebsocketProtocol, PingWithExtendedLengthIsRejected) {
    BufferReader reader{MakeExtendedControlFrame(WSOpcodes::kPing, kOversizedControlPayload)};

    std::string payload;
    FrameParserState frame;
    frame.payload = &payload;
    std::size_t payload_len = 0;

    EXPECT_EQ(ReadWSFrame(frame, reader, kMaxPayloadSize, payload_len), websocket::CloseStatus::kProtocolError);
}

UTEST(WebsocketProtocol, PongWithExtendedLengthIsRejected) {
    BufferReader reader{MakeExtendedControlFrame(WSOpcodes::kPong, kOversizedControlPayload)};

    std::string payload;
    FrameParserState frame;
    frame.payload = &payload;
    std::size_t payload_len = 0;

    EXPECT_EQ(ReadWSFrame(frame, reader, kMaxPayloadSize, payload_len), websocket::CloseStatus::kProtocolError);
}

UTEST(WebsocketProtocol, PingWithinControlFrameLimitIsAccepted) {
    BufferReader reader{MakeShortPingFrame(kMaxControlPayload)};

    std::string payload;
    FrameParserState frame;
    frame.payload = &payload;
    std::size_t payload_len = 0;

    EXPECT_EQ(ReadWSFrame(frame, reader, kMaxPayloadSize, payload_len), websocket::CloseStatus::kNone);
    EXPECT_TRUE(frame.ping_received);
    EXPECT_EQ(payload_len, kMaxControlPayload);
    EXPECT_EQ(payload, std::string(kMaxControlPayload, kPayloadFill));
}

// A ping is allowed to arrive in the middle of a fragmented data message, so
// the buffer holds the unfinished data too. Only the ping's own bytes may be
// echoed back in the pong.
UTEST(WebsocketProtocol, PingDuringFragmentedMessageKeepsDataSeparate) {
    BufferReader reader{
        MakeMaskedTextFragment(kInterleavedFragmentSize, Final::kNo) + MakeShortPingFrame(kInterleavedFragmentSize)
    };

    std::string payload;
    FrameParserState frame;
    frame.payload = &payload;
    std::size_t payload_len = 0;

    ASSERT_EQ(ReadWSFrame(frame, reader, kMaxPayloadSize, payload_len), websocket::CloseStatus::kNone);
    ASSERT_TRUE(frame.waiting_continuation);

    ASSERT_EQ(ReadWSFrame(frame, reader, kMaxPayloadSize, payload_len), websocket::CloseStatus::kNone);
    EXPECT_TRUE(frame.ping_received);
    EXPECT_EQ(payload_len, kInterleavedFragmentSize);
    EXPECT_EQ(payload.size(), kInterleavedFragmentSize * 2);
    // the pong payload is the tail of the buffer and stays within the limit
    EXPECT_EQ(payload.substr(payload.size() - payload_len), std::string(kInterleavedFragmentSize, kPayloadFill));
}

TEST(WebsocketProtocol, ControlFrameHeaderKeepsPayloadLength) {
    const std::string payload(kMaxControlPayload, kPayloadFill);
    const auto header = MakeControlFrame(WSOpcodes::kPong, payload.size(), Masked::kNo);

    WSHeader hdr{};
    std::memcpy(&hdr, header.data(), sizeof(hdr));
    EXPECT_EQ(hdr.bits.payload_len, kMaxControlPayload);
    EXPECT_EQ(hdr.bits.opcode, WSOpcodes::kPong);
    EXPECT_EQ(hdr.bits.fin, 1);
    EXPECT_EQ(hdr.bits.mask, 0);
}

TEST(WebsocketProtocol, DataFrameHeaderUsesMinimalPayloadLengthEncoding) {
    struct TestCase {
        std::size_t payload_size;
        std::vector<unsigned char> expected_header;
    };
    const std::array<TestCase, 8> cases = {{
        {.payload_size = 0, .expected_header = {0x81, 0x00}},
        {.payload_size = 125, .expected_header = {0x81, 0x7d}},
        {.payload_size = 126, .expected_header = {0x81, 0x7e, 0x00, 0x7e}},
        {.payload_size = 32767, .expected_header = {0x81, 0x7e, 0x7f, 0xff}},
        {.payload_size = 32768, .expected_header = {0x81, 0x7e, 0x80, 0x00}},
        {.payload_size = 38874, .expected_header = {0x81, 0x7e, 0x97, 0xda}},
        {.payload_size = 65535, .expected_header = {0x81, 0x7e, 0xff, 0xff}},
        {.payload_size = 65536, .expected_header = {0x81, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}},
    }};

    for (const auto& test_case : cases) {
        SCOPED_TRACE(test_case.payload_size);
        for (const auto masked : {Masked::kNo, Masked::kYes}) {
            SCOPED_TRACE(masked == Masked::kYes);
            const auto header = DataFrameHeader(test_case.payload_size, true, Continuation::kNo, Final::kYes, masked);
            auto expected_header = test_case.expected_header;
            if (masked == Masked::kYes) {
                expected_header[1] |= 0x80;
            }
            EXPECT_EQ(std::vector<unsigned char>(header.begin(), header.end()), expected_header);
        }
    }
}

USERVER_NAMESPACE_END
