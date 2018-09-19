#pragma once

#include <iostream>

#include <logging/level.hpp>

#include "log_config.hpp"

namespace logging {

class MessageBuffer : public std::streambuf {
 public:
  MessageBuffer(Level level);
  virtual ~MessageBuffer() = default;

 protected:
  std::streamsize xsputn(const char_type* s, std::streamsize n) override;

 public:
  spdlog::details::log_msg msg;
};

}  // namespace logging
