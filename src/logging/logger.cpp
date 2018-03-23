#include "logger.hpp"

#include <memory>

#include <spdlog/common.h>
#include <spdlog/formatter.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/file_sinks.h>

#include "config.hpp"

namespace logging {

namespace {

LoggerPtr MakeSimpleLogger(const std::string& name, spdlog::sink_ptr sink) {
  auto logger = std::make_shared<spdlog::logger>(name, sink);
  logger->set_formatter(std::make_shared<spdlog::pattern_formatter>(
      LoggerConfig::kDefaultPattern));
  logger->set_level(spdlog::level::level_enum::info);
  logger->flush_on(spdlog::level::level_enum::info);
  return logger;
}

}  // namespace

LoggerPtr MakeStderrLogger(const std::string& name) {
  return MakeSimpleLogger(name, spdlog::sinks::stderr_sink_mt::instance());
}

LoggerPtr MakeFileLogger(const std::string& name, const std::string& path) {
  return MakeSimpleLogger(
      name, std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                path, /*max_size=*/-1,
                /*max_files=*/0));
}

}  // namespace logging
