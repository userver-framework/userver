#include "log_streambuf.hpp"

#include <logging/log.hpp>

namespace logging {

namespace {

class PutCharFmtBuffer {
 public:
  void operator()(fmt::memory_buffer& to, char ch) const { to.push_back(ch); }
};

}  // namespace

MessageBuffer::MessageBuffer(Level level)
    : tskv_value_encode_(false),
      msg(&DefaultLogger()->name(),
          static_cast<spdlog::level::level_enum>(level)) {}

std::streamsize MessageBuffer::xsputn(const char_type* s, std::streamsize n) {
  if (!tskv_value_encode_) {
    msg.raw.append(s, s + n);
  } else {
    msg.raw.reserve(msg.raw.size() + n);
    utils::encoding::EncodeTskv(msg.raw, s, s + n,
                                utils::encoding::EncodeTskvMode::kValue,
                                PutCharFmtBuffer{});
  }
  return n;
}

MessageBuffer::int_type MessageBuffer::overflow(int_type c) {
  if (c == traits_type::eof()) return c;

  if (!tskv_value_encode_) {
    msg.raw.push_back(c);
  } else {
    utils::encoding::EncodeTskv(msg.raw, static_cast<char>(c),
                                utils::encoding::EncodeTskvMode::kValue,
                                PutCharFmtBuffer{});
  }

  return c;
}

}  // namespace logging
