#pragma once

#include <iostream>

#include <logging/level.hpp>
#include <utils/encoding/tskv.hpp>

#include "log_config.hpp"

namespace logging {

/// @brief Stream buffer that tskv-encodes values
class TskvBuffer : public std::streambuf {
 public:
  TskvBuffer(fmt::memory_buffer& buff,
             utils::encoding::EncodeTskvMode mode =
                 utils::encoding::EncodeTskvMode::kValue)
      : buffer_{buff}, mode_{mode} {}
  virtual ~TskvBuffer() = default;

 protected:
  std::streamsize xsputn(const char_type* s, std::streamsize n) override;

 private:
  fmt::memory_buffer& buffer_;
  utils::encoding::EncodeTskvMode mode_;
};

}  // namespace logging
