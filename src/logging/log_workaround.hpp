#pragma once

#include "log_config.hpp"

namespace logging {

class LoggerWorkaroud : public spdlog::logger {
 public:
  using spdlog::logger::sink_it_;
};

}  // namespace logging
