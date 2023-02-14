#pragma once

#include <gtest/gtest.h>
#include <spdlog/sinks/ostream_sink.h>

#include <logging/config.hpp>
#include <logging/spdlog_helpers.hpp>
#include <logging/tp_logger.hpp>
#include <logging/unix_socket_sink.hpp>

#include <userver/logging/format.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

inline std::shared_ptr<logging::impl::TpLogger> MakeLoggerFromSink(
    const std::string& logger_name,
    std::shared_ptr<spdlog::sinks::sink> sink_ptr, logging::Format format) {
  auto logger = std::make_shared<logging::impl::TpLogger>(format, logger_name);
  logger->AddSink(std::move(sink_ptr));
  logger->SetPattern(logging::GetSpdlogPattern(format));
  return logger;
}

inline std::shared_ptr<logging::impl::TpLogger> MakeNamedStreamLogger(
    const std::string& logger_name, std::ostream& stream,
    logging::Format format) {
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(stream);
  return MakeLoggerFromSink(logger_name, sink, format);
}

inline std::string_view GetTextKey(logging::Format format) {
  return format == logging::Format::kLtsv ? "text:" : "text=";
}

inline std::string ParseLoggedText(std::string_view log_record,
                                   logging::Format format) {
  const auto text_key = GetTextKey(format);

  const auto text_begin = log_record.find(text_key);
  if (text_begin != std::string_view::npos) {
    log_record.remove_prefix(text_begin + text_key.length());
  }

  const auto log_end = log_record.find_first_of("\r\n");
  if (log_end != std::string_view::npos &&
      log_record.substr(log_end).size() > 2) {
    throw std::runtime_error("The log contains multiple log records");
  }

  const auto text_end = log_record.find_first_of("\t\r\n");
  if (text_end != std::string_view::npos) {
    log_record = log_record.substr(0, text_end);
  }

  return std::string{log_record};
}

class DefaultLoggerFixture : public ::testing::Test {
 protected:
  ~DefaultLoggerFixture() override {
    if (old_logger_) {
      // Discard logs from within SetDefaultLogger
      logging::SetDefaultLoggerLevel(logging::Level::kNone);
      logging::LogFlush();

      logging::SetDefaultLogger(old_logger_);
      old_logger_.reset();
    }
  }

  void SetDefaultLogger(logging::LoggerPtr new_logger) {
    // Ignore clang-tidy request to make the method 'static'
    [[maybe_unused]] auto* const self = this;

    const auto old_level = logging::GetLoggerLevel(new_logger);
    // Discard logs from within SetLoggerLevel itself
    logging::SetLoggerLevel(new_logger, logging::Level::kNone);

    logging::SetDefaultLogger(new_logger);
    logging::SetLoggerLevel(new_logger, old_level);
  }

 private:
  logging::LoggerPtr old_logger_{logging::DefaultLogger()};
};

class LoggingTestBase : public DefaultLoggerFixture {
 protected:
  LoggingTestBase(logging::Format format) : format_(format) {
    stream_logger_ = MakeStreamLogger(sstream_);
  }

  std::string LoggedText() const {
    logging::LogFlush();
    return ParseLoggedText(sstream_.str(), format_);
  }

  bool LoggedTextContains(std::string_view str) const {
    return LoggedText().find(str) != std::string::npos;
  }

  void ClearLog() { sstream_.str({}); }

  template <typename T>
  std::string ToStringViaLogging(const T& value) {
    LOG_CRITICAL() << value;
    std::string result = LoggedText();
    ClearLog();
    return result;
  }

  std::shared_ptr<logging::impl::TpLogger> MakeStreamLogger(
      std::ostream& stream) const {
    return MakeNamedStreamLogger("test-stream-logger", stream, format_);
  }

  std::string GetStreamString() const { return sstream_.str(); }

  std::size_t GetRecordsCount() const {
    auto str = sstream_.str();
    return std::count(str.begin(), str.end(), '\n');
  }

  std::shared_ptr<logging::impl::TpLogger> GetStreamLogger() const {
    return stream_logger_;
  }

 private:
  std::shared_ptr<logging::impl::TpLogger> stream_logger_;

  const logging::Format format_;
  std::ostringstream sstream_;
};

class LoggingTest : public LoggingTestBase {
 protected:
  LoggingTest() : LoggingTestBase(logging::Format::kTskv) {
    SetDefaultLogger(GetStreamLogger());
  }
};

class LoggingLtsvTest : public LoggingTestBase {
 protected:
  LoggingLtsvTest() : LoggingTestBase(logging::Format::kLtsv) {
    SetDefaultLogger(GetStreamLogger());
  }
};

USERVER_NAMESPACE_END
