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

}  // namespace logging
