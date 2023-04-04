#include <userver/logging/logger.hpp>

#include <memory>

#include <logging/reopening_file_sink.hpp>
#include <logging/spdlog.hpp>
#include <logging/tp_logger.hpp>
#include <logging/unix_socket_sink.hpp>

#include <spdlog/formatter.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#include "config.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {

LoggerPtr MakeSimpleLogger(const std::string& name, spdlog::sink_ptr sink,
                           Level level, Format format) {
  auto logger = std::make_shared<impl::TpLogger>(format, name);
  logger->SetLevel(level);

  if (sink) {
    logger->AddSink(std::move(sink));
  }

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
  return MakeSimpleLogger(name, MakeStderrSink(), level, format);
}

LoggerPtr MakeStdoutLogger(const std::string& name, Format format,
                           Level level) {
  return MakeSimpleLogger(name, MakeStdoutSink(), level, format);
}

LoggerPtr MakeFileLogger(const std::string& name, const std::string& path,
                         Format format, Level level) {
  return MakeSimpleLogger(name,
                          std::make_shared<logging::ReopeningFileSinkMT>(path),
                          level, format);
}

namespace impl {

void LogRaw(LoggerBase& logger, Level level, std::string_view message) {
  logger.Log(level, message);
}

}  // namespace impl

}  // namespace logging

USERVER_NAMESPACE_END
