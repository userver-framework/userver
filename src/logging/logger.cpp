#include <logging/logger.hpp>

#include <memory>

// this header must be included before any spdlog headers
// to override spdlog's level names
#include <logging/spdlog.hpp>

#include <spdlog/formatter.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#include "config.hpp"

namespace logging {

namespace {

LoggerPtr MakeSimpleLogger(const std::string& name, spdlog::sink_ptr sink) {
  auto logger = std::make_shared<spdlog::logger>(name, sink);
  logger->set_formatter(std::make_unique<spdlog::pattern_formatter>(
      LoggerConfig::kDefaultPattern));
  logger->set_level(spdlog::level::level_enum::info);
  logger->flush_on(spdlog::level::level_enum::info);
  return logger;
}

spdlog::sink_ptr MakeStderrSink() {
  static auto sink = std::make_shared<spdlog::sinks::stderr_sink_mt>();
  return sink;
}

}  // namespace

LoggerPtr MakeStderrLogger(const std::string& name) {
  return MakeSimpleLogger(name, MakeStderrSink());
}

LoggerPtr MakeFileLogger(const std::string& name, const std::string& path) {
  return MakeSimpleLogger(
      name, std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                path, /*max_size=*/-1,
                /*max_files=*/0));
}

}  // namespace logging
