#include <server/websocket/protocol.hpp>

#include <cstdlib>
#include <cstring>

#include <cryptopp/sha.h>
#include <boost/endian/conversion.hpp>

#include <userver/crypto/base64.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::websocket::impl {

namespace {

inline void RecvExactly(engine::io::ReadableBase& readable,
                        utils::span<char> buffer, engine::Deadline deadline) {
  if (readable.ReadAll(buffer.data(), buffer.size(), deadline) != buffer.size())
    throw(engine::io::IoException() << "Socket closed during transfer ");
}

template <class T>
utils::span<char> AsWritableBytes(utils::span<T> s) noexcept {
  return utils::span<char>{reinterpret_cast<char*>(s.data()),
                           reinterpret_cast<char*>(s.data() + s.size())};
}

template <class T>
utils::span<T> MakeSpan(T* ptr, size_t count) {
  return utils::span<T>(ptr, ptr + count);
}

union Mask32 {
  uint32_t mask32 = 0;
  uint8_t mask8[4];
};

void XorMaskInplace(uint8_t* dest, size_t len, Mask32 mask) {
  auto* dest32 = reinterpret_cast<uint32_t*>(dest);
  while (len >= sizeof(uint32_t)) {
    *(dest32++) ^= mask.mask32;
    len -= sizeof(uint32_t);
  }
  auto* dest8 = reinterpret_cast<uint8_t*>(dest32);
  for (unsigned i = 0; i < len; ++i) *(dest8++) ^= mask.mask8[i];
}

template <class T, class V>
void PushRaw(const T& value, V& data) {
  const auto* valBytes = reinterpret_cast<const char*>(&value);
  data.insert(data.end(), valBytes, valBytes + sizeof(value));
}

}  // namespace

namespace frames {

boost::container::small_vector<char, impl::kMaxFrameHeaderSize> DataFrameHeader(
    utils::span<const std::byte> data, bool is_text,
    Continuation is_continuation, Final is_final) {
  boost::container::small_vector<char, impl::kMaxFrameHeaderSize> frame;

  frame.resize(sizeof(WSHeader));
  auto* hdr = reinterpret_cast<WSHeader*>(frame.data());
  UASSERT_MSG(
      reinterpret_cast<std::uintptr_t>(frame.data()) % alignof(WSHeader) == 0,
      "Misaligned");

  hdr->bytes = 0;
  hdr->bits.fin = is_final == Final::kYes ? 1 : 0;
  hdr->bits.opcode = is_text ? kText : kBinary;
  if (is_continuation == Continuation::kYes) hdr->bits.opcode = kContinuation;

  if (data.size() <= 125) {
    hdr->bits.payloadLen = data.size();
  } else if (data.size() <= INT16_MAX) {
    hdr->bits.payloadLen = 126;
    PushRaw(
        boost::endian::native_to_big(static_cast<std::int16_t>(data.size())),
        frame);
  } else {
    hdr->bits.payloadLen = 127;
    PushRaw(boost::endian::native_to_big(data.size()), frame);
  }
  return frame;
}

std::string CloseFrame(CloseStatusInt status_code) {
  std::string frame;
  frame.resize(sizeof(WSHeader) + sizeof(status_code));
  auto* hdr = reinterpret_cast<WSHeader*>(frame.data());
  hdr->bits.fin = 1;
  hdr->bits.opcode = kClose;
  hdr->bits.payloadLen = sizeof(status_code);

  auto* payload = reinterpret_cast<CloseStatusInt*>(&frame[sizeof(WSHeader)]);
  *payload = boost::endian::native_to_big(status_code);
  return frame;
}

std::array<char, sizeof(WSHeader)> MakeControlFrame(
    WSOpcodes opcode, utils::span<const std::byte> data) {
  std::array<char, sizeof(WSHeader)> frame{};

  auto* hdr = reinterpret_cast<WSHeader*>(frame.data());
  hdr->bytes = 0;
  hdr->bits.fin = 1;
  hdr->bits.opcode = opcode;

  hdr->bits.payloadLen = data.size();
  return frame;
}

const std::array<char, sizeof(WSHeader)> ws_ping_frame =
    MakeControlFrame(kPing);
const std::array<char, sizeof(WSHeader)> ws_close_frame =
    MakeControlFrame(kClose);

const std::array<char, sizeof(WSHeader)>& PingFrame() { return ws_ping_frame; }
const std::array<char, sizeof(WSHeader)>& CloseFrame() {
  return ws_close_frame;
}

}  // namespace frames

std::string WebsocketSecAnswer(std::string_view sec_key) {
  // guid is taken from RFC
  // https://datatracker.ietf.org/doc/html/rfc6455#section-1.3
  static constexpr std::string_view websocketGuid =
      "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

#if defined(CRYPTOPP_NO_GLOBAL_BYTE)
  using CryptoByte = CryptoPP::byte;
#else
  using CryptoByte = ::byte;
#endif

  CryptoByte webSocketRespKeySHA1[CryptoPP::SHA1::DIGESTSIZE];
  CryptoPP::SHA1 hash;
  hash.Update(reinterpret_cast<const CryptoByte*>(sec_key.data()),
              sec_key.size());
  hash.Update(reinterpret_cast<const CryptoByte*>(websocketGuid.data()),
              websocketGuid.size());
  hash.Final(webSocketRespKeySHA1);
  return crypto::base64::Base64Encode(
      std::string_view(reinterpret_cast<const char*>(webSocketRespKeySHA1),
                       sizeof(webSocketRespKeySHA1)));
}

CloseStatus ReadWSFrame(FrameParserState& frame, engine::io::ReadableBase& io,
                        unsigned max_payload_size, std::size_t& payload_len) {
  WSHeader hdr;
  RecvExactly(io, AsWritableBytes(MakeSpan(&hdr, 1)), {});
  if (engine::current_task::ShouldCancel()) return CloseStatus::kGoingAway;

  const bool isDataFrame =
      (hdr.bits.opcode & (kText | kBinary)) || hdr.bits.opcode == kContinuation;
  if (hdr.bits.payloadLen <= 125) {
    payload_len = hdr.bits.payloadLen;
  } else if (hdr.bits.payloadLen == 126) {
    uint16_t payloadLen16 = 0;
    RecvExactly(io, AsWritableBytes(MakeSpan(&payloadLen16, 1)), {});
    payload_len = boost::endian::big_to_native(payloadLen16);
  } else  // if (hdr.payloadLen == 127)
  {
    uint64_t payloadLen64 = 0;
    RecvExactly(io, AsWritableBytes(MakeSpan(&payloadLen64, 1)), {});
    payload_len = boost::endian::big_to_native(payloadLen64);
  }
  if (engine::current_task::ShouldCancel()) return CloseStatus::kGoingAway;

  if (!isDataFrame && hdr.bits.payloadLen > 125) {
    // control frame should not have extended payload
    return CloseStatus::kProtocolError;
  }

  if (payload_len + frame.payload->size() > max_payload_size)
    return CloseStatus::kTooBigData;

  Mask32 mask;
  if (hdr.bits.mask) RecvExactly(io, AsWritableBytes(MakeSpan(&mask, 1)), {});
  if (engine::current_task::ShouldCancel()) return CloseStatus::kGoingAway;

  if (payload_len > 0) {
    if (isDataFrame) {
      if (!frame.payload->empty() && hdr.bits.opcode != kContinuation) {
        // non-continuation opcode while waiting continuation
        return CloseStatus::kProtocolError;
      }
    }

    size_t newPayloadOffset = frame.payload->size();
    frame.payload->resize(frame.payload->size() + payload_len);
    RecvExactly(io,
                MakeSpan(frame.payload->data() + newPayloadOffset, payload_len),
                {});
    if (engine::current_task::ShouldCancel()) return CloseStatus::kGoingAway;

    if (mask.mask32)
      XorMaskInplace(reinterpret_cast<uint8_t*>(frame.payload->data()),
                     frame.payload->size(), mask);
  }
  char opcode = hdr.bits.opcode;
  char fin = hdr.bits.fin;

  switch (opcode) {
    case kPing:
      frame.ping_received = true;
      break;
    case kPong:
      frame.pong_received = true;
      break;
    case kClose:
      frame.closed = true;
      if (frame.payload->size() >= 2)
        frame.remote_close_status = boost::endian::big_to_native(
            *(reinterpret_cast<CloseStatusInt const*>(frame.payload->data())));
      break;
    case kText:
      frame.is_text = true;
      [[fallthrough]];
    case kBinary:
    case kContinuation:
      frame.waiting_continuation = !fin;
      break;
    default:
      // unknown opcode
      return CloseStatus::kProtocolError;
  }
  return CloseStatus::kNone;
}

}  // namespace server::websocket::impl

USERVER_NAMESPACE_END
