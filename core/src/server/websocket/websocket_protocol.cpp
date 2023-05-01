#include "websocket_protocol.h"
#include <cryptopp/sha.h>
#include <endian.h>
#include <cstdlib>
#include <cstring>

#include <userver/crypto/base64.hpp>
#include <userver/engine/task/cancel.hpp>

#include "utils.h"

namespace userver::websocket {

using utils::impl::Span;

enum WSOpcodes {
  CONTINUATION = 0,
  TEXT = 0x1,
  BINARY = 0x2,
  CLOSE = 0x8,
  PING = 0x9,
  PONG = 0xA
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
static constexpr unsigned int max_frame_header_size =
    sizeof(WSHeader) + sizeof(uint64_t);

// Do not forget to replace with start_lifetime_as in C++23
template <class To, class From>
To union_conv_ptr(From ptr) {
  union {
    To t;
    From f;
  } ptrUn;
  ptrUn.f = ptr;

  // Yes I know that reading field which had not been written is UB. but what to
  // do?
  return ptrUn.t;
}

union Mask32 {
  uint32_t mask32 = 0;
  uint8_t mask8[4];
};

void xor_mask_inplace(uint8_t* dest, size_t len, Mask32 mask) {
  uint32_t* dest32 = union_conv_ptr<uint32_t*>(dest);
  while (len >= sizeof(uint32_t)) {
    *(dest32++) ^= mask.mask32;
    len -= sizeof(uint32_t);
  }
  uint8_t* dest8 = union_conv_ptr<uint8_t*>(dest32);
  for (int i = 0; i < (int)len; ++i) *(dest8++) ^= mask.mask8[i];
}

template <class T>
static void push_raw(const T& value, std::vector<std::byte>& data) {
  const std::byte* valBytes = union_conv_ptr<const std::byte*>(&value);
  data.insert(data.end(), valBytes, valBytes + sizeof(value));
}

std::vector<std::byte> frames::DataFrame(
    Span<const std::byte> data, bool is_text, bool is_continuation,
    bool is_final) {
  std::vector<std::byte> frame;
  frame.reserve(data.size() + max_frame_header_size);

  frame.resize(sizeof(WSHeader));
  WSHeader* hdr = union_conv_ptr<WSHeader*>(frame.data());
  hdr->bytes = 0;
  hdr->bits.fin = is_final ? 1 : 0;
  hdr->bits.opcode = is_text ? TEXT : BINARY;
  if (is_continuation) hdr->bits.opcode = CONTINUATION;

  if (data.size() <= 125) {
    hdr->bits.payloadLen = data.size();
  } else if (data.size() <= INT16_MAX) {
    hdr->bits.payloadLen = 126;
    push_raw(htobe16(data.size()), frame);
  } else {
    hdr->bits.payloadLen = 127;
    push_raw(htobe64(data.size()), frame);
  }
  frame.insert(frame.end(), data.begin(), data.end());
  return frame;
}

std::vector<std::byte> frames::CloseFrame(CloseStatusInt status_code) {
  std::vector<std::byte> frame;
  frame.resize(sizeof(WSHeader) + sizeof(status_code));
  WSHeader* hdr = union_conv_ptr<WSHeader*>(frame.data());
  hdr->bits.fin = 1;
  hdr->bits.opcode = CLOSE;
  hdr->bits.payloadLen = sizeof(status_code);

  CloseStatusInt* payload = (CloseStatusInt*)(&frame[sizeof(WSHeader)]);
  *payload = htobe16(status_code);
  return frame;
}

static std::vector<std::byte> ws_ping_frame, ws_pong_frame, ws_close_frame;

static void make_control_frame(WSOpcodes opcode,
                               std::vector<std::byte>& frame) {
  frame.resize(sizeof(WSHeader));
  WSHeader* hdr = union_conv_ptr<WSHeader*>(frame.data());
  hdr->bytes = 0;
  hdr->bits.fin = 1;
  hdr->bits.opcode = opcode;
}

static struct InitControlFrames {
  InitControlFrames() {
    make_control_frame(CLOSE, ws_close_frame);
    make_control_frame(PING, ws_ping_frame);
    make_control_frame(PONG, ws_pong_frame);
  }
} init_control_frames;

const std::vector<std::byte>& frames::PingFrame() { return ws_ping_frame; }
const std::vector<std::byte>& frames::PongFrame() { return ws_pong_frame; }
const std::vector<std::byte>& frames::CloseFrame() { return ws_close_frame; }

std::string WebsocketSecAnswer(std::string_view sec_key) {
  // guid is taken from RFC
  // https://datatracker.ietf.org/doc/html/rfc6455#section-1.3
  static const std::string_view websocket_guid =
      "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  CryptoPP::byte webSocketRespKeySHA1[CryptoPP::SHA1::DIGESTSIZE];
  CryptoPP::SHA1 hash;
  hash.Update((const CryptoPP::byte*)sec_key.data(), sec_key.size());
  hash.Update((const CryptoPP::byte*)websocket_guid.data(),
              websocket_guid.size());
  hash.Final(webSocketRespKeySHA1);
  return userver::crypto::base64::Base64Encode(
      std::string_view((const char*)webSocketRespKeySHA1, 20));
}

CloseStatusInt ReadWSFrame(FrameParserState& frame,
                           userver::engine::io::ReadableBase* io,
                           unsigned max_payload_size) {
  WSHeader hdr;
  RecvExactly(io, as_writable_bytes(make_span(&hdr, 1)), {});
  if (userver::engine::current_task::ShouldCancel())
    return (CloseStatusInt)CloseStatus::kGoingAway;

  const bool isDataFrame =
      (hdr.bits.opcode & (TEXT | BINARY)) || hdr.bits.opcode == CONTINUATION;
  uint64_t payloadLen = 0;
  if (hdr.bits.payloadLen <= 125) {
    payloadLen = hdr.bits.payloadLen;
  } else if (hdr.bits.payloadLen == 126) {
    uint16_t payloadLen16;
    RecvExactly(io, as_writable_bytes(make_span(&payloadLen16, 1)), {});
    payloadLen = be16toh(payloadLen16);
  } else  // if (hdr.payloadLen == 127)
  {
    uint64_t payloadLen64;
    RecvExactly(io, as_writable_bytes(make_span(&payloadLen64, 1)), {});
    payloadLen = be64toh(payloadLen64);
  }
  if (userver::engine::current_task::ShouldCancel())
    return (CloseStatusInt)CloseStatus::kGoingAway;

  if (!isDataFrame && hdr.bits.payloadLen > 125) {
    // control frame should not have extended payload
    return (CloseStatusInt)CloseStatus::kProtocolError;
  }

  if (payloadLen + frame.payload.size() > max_payload_size)
    return (CloseStatusInt)CloseStatus::kTooBigData;

  Mask32 mask;
  if (hdr.bits.mask)
    RecvExactly(io, as_writable_bytes(make_span(&mask, 1)), {});
  if (userver::engine::current_task::ShouldCancel())
    return (CloseStatusInt)CloseStatus::kGoingAway;

  if (payloadLen > 0) {
    if (isDataFrame) {
      if (!frame.payload.empty() && hdr.bits.opcode != CONTINUATION) {
        // non-continuation opcode while waiting continuation
        return (CloseStatusInt)CloseStatus::kProtocolError;
      }
    }

    size_t newPayloadOffset = frame.payload.size();
    frame.payload.resize(frame.payload.size() + payloadLen);
    RecvExactly(
        io, make_span(frame.payload.data() + newPayloadOffset, payloadLen), {});
    if (userver::engine::current_task::ShouldCancel())
      return (CloseStatusInt)CloseStatus::kGoingAway;

    if (mask.mask32)
      xor_mask_inplace((uint8_t*)frame.payload.data(), frame.payload.size(),
                       mask);
  }
  char opcode = hdr.bits.opcode;
  char fin = hdr.bits.fin;

  switch (opcode) {
    case PING:
      frame.pingReceived = true;
      break;
    case PONG:
      frame.pongReceived = true;
      break;
    case CLOSE:
      frame.closed = true;
      if (frame.payload.size() >= 2)
        frame.remoteCloseStatus = be16toh(
            *(union_conv_ptr<CloseStatusInt const*>(frame.payload.data())));
      break;
    case TEXT:
      frame.isText = true;
      [[fallthrough]];
    case BINARY:
    case CONTINUATION:
      frame.waitingContinuation = !fin;
      break;
    default:
      // unknown opcode
      return (CloseStatusInt)CloseStatus::kProtocolError;
  }
  return 0;
}

}  // namespace userver::websocket
