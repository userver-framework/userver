#include "logger.hpp"

#include <spdlog/logger.h>

namespace logging {

LoggerPtr MakeStderrLogger(const std::string& name) {
  return spdlog::stderr_logger_mt(name);
}

LoggerPtr MakeFileLogger(const std::string& name, const std::string& path) {
  return spdlog::rotating_logger_mt(name, path, /*max_file_size=*/-1,
                                    /*max_files=*/0);
}

}  // namespace logging
