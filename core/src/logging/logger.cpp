#include <userver/logging/logger.hpp>

#include <memory>

// this header must be included before any spdlog headers
// to override spdlog's level names
#include <logging/spdlog.hpp>

#include <logging/logger_with_info.hpp>
#include <logging/reopening_file_sink.hpp>

#include <spdlog/formatter.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#include "config.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {

LoggerPtr MakeSimpleLogger(const std::string& name, spdlog::sink_ptr sink,
                           spdlog::level::level_enum level, Format format) {
  auto spdlog_logger = utils::MakeSharedRef<spdlog::logger>(name, sink);
  auto logger = std::make_shared<impl::LoggerWithInfo>(
      format, std::shared_ptr<spdlog::details::thread_pool>{},
      std::move(spdlog_logger));

  std::string pattern;
  switch (format) {
    case Format::kTskv:
      pattern = LoggerConfig::kDefaultTskvPattern;
      break;
    case Format::kLtsv:
      pattern = LoggerConfig::kDefaultLtsvPattern;
      break;
  }
  logger->ptr->set_formatter(
      std::make_unique<spdlog::pattern_formatter>(std::move(pattern)));

  logger->ptr->set_level(level);
  logger->ptr->flush_on(level);
  return logger;
}

spdlog::sink_ptr MakeStderrSink() {
  static auto sink = std::make_shared<spdlog::sinks::stderr_sink_mt>();
  return sink;
}

spdlog::sink_ptr MakeStdoutSink() {
  static auto sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
  return sink;
}

}  // namespace

LoggerPtr MakeStderrLogger(const std::string& name, Format format,
                           Level level) {
  return MakeSimpleLogger(name, MakeStderrSink(),
                          static_cast<spdlog::level::level_enum>(level),
                          format);
}

LoggerPtr MakeStdoutLogger(const std::string& name, Format format,
                           Level level) {
  return MakeSimpleLogger(name, MakeStdoutSink(),
                          static_cast<spdlog::level::level_enum>(level),
                          format);
}

LoggerPtr MakeFileLogger(const std::string& name, const std::string& path,
                         Format format, Level level) {
  return MakeSimpleLogger(
      name, std::make_shared<logging::ReopeningFileSinkMT>(path),
      static_cast<spdlog::level::level_enum>(level), format);
}

LoggerPtr MakeNullLogger(const std::string& name) {
  return MakeSimpleLogger(name, std::make_shared<spdlog::sinks::null_sink_mt>(),
                          spdlog::level::off, Format::kTskv);
}

namespace impl {

void LogRaw(LoggerWithInfo& logger, Level level, std::string_view message) {
  auto spdlog_level = static_cast<spdlog::level::level_enum>(level);
  logger.ptr->log(spdlog_level, "{}", message);
}

}  // namespace impl

}  // namespace logging

USERVER_NAMESPACE_END
