#pragma once

#include <logging/spdlog.hpp>

namespace logging {

class LoggerWorkaroud : public spdlog::logger {
 public:
  using spdlog::logger::sink_it_;
};

}  // namespace logging
