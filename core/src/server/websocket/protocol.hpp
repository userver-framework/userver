#pragma once

#include <userver/server/websocket/server.hpp>

#include <string>

#include <boost/container/small_vector.hpp>

#include <userver/engine/io/common.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::websocket::impl {

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

    unsigned char payloadLen : 7;
    unsigned char mask : 1;
  } bits;
  uint16_t bytes = 0;
};

static_assert(sizeof(WSHeader) == 2);

constexpr inline unsigned int kMaxFrameHeaderSize =
    sizeof(WSHeader) + sizeof(uint64_t);

namespace frames {

enum class Continuation {
  kYes,
  kNo,
};

enum class Final {
  kYes,
  kNo,
};

boost::container::small_vector<char, impl::kMaxFrameHeaderSize> DataFrameHeader(
    utils::span<const std::byte> data, bool is_text,
    Continuation is_continuation, Final is_final);
std::array<char, sizeof(WSHeader)> MakeControlFrame(
    WSOpcodes opcode, utils::span<const std::byte> data = {});
std::string CloseFrame(CloseStatusInt status_code);

const std::array<char, sizeof(WSHeader)>& PingFrame();
const std::array<char, sizeof(WSHeader)>& CloseFrame();
}  // namespace frames

std::string WebsocketSecAnswer(std::string_view sec_key);

struct FrameParserState {
  bool closed = false;
  bool ping_received = false;
  bool pong_received = false;
  bool waiting_continuation = false;
  bool is_text = false;
  CloseStatusInt remote_close_status = 0;

  std::string* payload = nullptr;
};

CloseStatus ReadWSFrame(FrameParserState& frame, engine::io::ReadableBase& io,
                        unsigned max_payload_size, std::size_t& payload_len);

}  // namespace server::websocket::impl

USERVER_NAMESPACE_END
