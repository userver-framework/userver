#include <userver/logging/logger.hpp>

#include <memory>

#include <logging/impl/buffered_file_sink.hpp>
#include <logging/impl/fd_sink.hpp>
#include <logging/impl/reopening_file_sink.hpp>
#include <logging/impl/unix_socket_sink.hpp>
#include <logging/spdlog.hpp>
#include <logging/tp_logger.hpp>

#include <spdlog/formatter.h>

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
  static auto sink = std::make_shared<impl::BufferedStderrFileSink>();
  return sink;
}

spdlog::sink_ptr MakeStdoutSink() {
  static auto sink = std::make_shared<impl::BufferedStdoutFileSink>();
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
  // TODO: On experiment
  if (logging::impl::kUseUserverSinks.IsEnabled()) {
    return MakeSimpleLogger(
        name, std::make_shared<impl::BufferedFileSink>(path), level, format);
  } else {
    return MakeSimpleLogger(
        name, std::make_shared<impl::ReopeningFileSinkMT>(path), level, format);
  }
}

namespace impl {

void LogRaw(LoggerBase& logger, Level level, std::string_view message) {
  logger.Log(level, message);
}

}  // namespace impl

}  // namespace logging

USERVER_NAMESPACE_END
