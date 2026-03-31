#pragma once

#include <optional>
#include <string>

#include <boost/container/small_vector.hpp>

#include <userver/engine/io/common.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/span.hpp>
#include <userver/websocket/message.hpp>

USERVER_NAMESPACE_BEGIN

namespace websocket::impl {

enum WSOpcodes {
    kContinuation = 0,
    kText = 0x1,
    kBinary = 0x2,
    kClose = 0x8,
    kPing = 0x9,
    kPong = 0xA,
};

/*
 https://datatracker.ietf.org/doc/html/rfc6455#section-5.2

      0                   1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-------+-+-------------+-------------------------------+
     |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
     |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
     |N|V|V|V|       |S|             |   (if payload len==126/127)   |
     | |1|2|3|       |K|             |                               |
     +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
     |     Extended payload length continued, if payload len == 127  |
     + - - - - - - - - - - - - - - - +-------------------------------+
     |                               |Masking-key, if MASK set to 1  |
     +-------------------------------+-------------------------------+
     | Masking-key (continued)       |          Payload Data         |
     +-------------------------------- - - - - - - - - - - - - - - - +
     :                     Payload Data continued ...                :
     + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
     |                     Payload Data continued ...                |
     +---------------------------------------------------------------+
*/

union WSHeader {
    struct {
        unsigned char opcode : 4;
        unsigned char reserved : 3;
        unsigned char fin : 1;

        unsigned char payload_len : 7;
        unsigned char mask : 1;
    } bits;
    uint16_t bytes = 0;
};

static_assert(sizeof(WSHeader) == 2);

constexpr inline unsigned int kMaxFrameHeaderSize = sizeof(WSHeader) + sizeof(uint64_t);

namespace frames {

enum class Continuation {
    kYes,
    kNo,
};

enum class Final {
    kYes,
    kNo,
};

enum class Masked {
    kYes,
    kNo,
};

union Mask32 {
    uint32_t mask32 = 0;
    uint8_t mask8[4];

    /// @brief Generate random 4-byte masking key
    static Mask32 Generate();
};

/// @brief Apply WebSocket masking to payload
/// @param payload Data to mask (modified in-place)
/// @param mask 4-byte masking key
void ApplyMask(utils::span<std::byte> payload, Mask32 mask);

boost::container::small_vector<char, impl::kMaxFrameHeaderSize> DataFrameHeader(
    utils::span<const std::byte> data,
    bool is_text,
    Continuation is_continuation,
    Final is_final,
    Masked is_masked
);

std::array<char, sizeof(WSHeader)> MakeControlFrame(
    WSOpcodes opcode,
    utils::span<const std::byte> data,
    Masked is_masked
);

}  // namespace frames

std::string WebsocketSecAnswer(std::string_view sec_key);

struct FrameParserState {
    bool closed = false;
    bool ping_received = false;
    bool pong_received = false;
    bool waiting_continuation = false;
    bool is_text = false;
    CloseStatusInt remote_close_status = 0;
    size_t offset_when_noblock = 0;

    std::string* payload = nullptr;
};

CloseStatus ReadWSFrame(
    FrameParserState& frame,
    engine::io::ReadableBase& io,
    unsigned max_payload_size,
    std::size_t& payload_len
);

std::optional<CloseStatus> ReadWSFrameDontWaitForHeader(
    FrameParserState& frame,
    engine::io::ReadableBase& io,
    unsigned max_payload_size,
    std::size_t& payload_len
);

}  // namespace websocket::impl

USERVER_NAMESPACE_END
