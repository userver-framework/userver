#include "log_streambuf.hpp"

#include <logging/log.hpp>

namespace logging {

MessageBuffer::MessageBuffer(Level level)
    : msg(&DefaultLogger()->name(),
          static_cast<spdlog::level::level_enum>(level)) {}

std::streamsize MessageBuffer::xsputn(const char_type* s, std::streamsize n) {
  msg.raw.append(s, s + n);
  return n;
}

MessageBuffer::int_type MessageBuffer::overflow(int_type c) {
  if (c != traits_type::eof()) msg.raw.push_back(c);
  return c;
}

}  // namespace logging
