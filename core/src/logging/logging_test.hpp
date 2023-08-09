#pragma once

#include <ostream>

#include <fmt/format.h>
#include <gtest/gtest.h>

#include <logging/config.hpp>
#include <logging/impl/unix_socket_sink.hpp>
#include <logging/tp_logger.hpp>

#include <userver/logging/format.hpp>
#include <userver/logging/log.hpp>
#include <userver/utest/default_logger_fixture.hpp>

USERVER_NAMESPACE_BEGIN

class StringSink final : public logging::impl::BaseSink {
 public:
  StringSink() = default;

  void Write(std::string_view log) final {
    ostream_.write(log.data(), log.size());
  }

  void Flush() final { ostream_.flush(); }

  std::ostringstream& GetStream() { return ostream_; }

 private:
  std::ostringstream ostream_;
};

inline std::shared_ptr<logging::impl::TpLogger> MakeLoggerFromSink(
    const std::string& logger_name, logging::impl::SinkPtr sink_ptr,
    logging::Format format) {
  auto logger = std::make_shared<logging::impl::TpLogger>(format, logger_name);
  logger->AddSink(std::move(sink_ptr));
  return logger;
}

struct StringStreamLogger final {
  std::shared_ptr<logging::impl::TpLogger> logger;
  std::ostringstream& stream;
};

inline StringStreamLogger MakeNamedStreamLogger(const std::string& logger_name,
                                                logging::Format format) {
  auto sink = std::make_unique<StringSink>();
  auto& stream = sink->GetStream();
  return {MakeLoggerFromSink(logger_name, std::move(sink), format), stream};
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
    throw std::runtime_error(
        fmt::format("The log contains multiple log records: {}", log_record));
  }

  const auto text_end = log_record.find_first_of("\t\r\n");
  if (text_end != std::string_view::npos) {
    log_record = log_record.substr(0, text_end);
  }

  return std::string{log_record};
}

using DefaultLoggerFixture = utest::DefaultLoggerFixture< ::testing::Test>;

class LoggingTestBase : public DefaultLoggerFixture {
 protected:
  explicit LoggingTestBase(logging::Format format)
      : format_(format),
        stream_logger_(MakeNamedStreamLogger("test-stream-logger", format_)) {}

  std::string LoggedText() const {
    logging::LogFlush();
    return ParseLoggedText(GetStreamString(), format_);
  }

  bool LoggedTextContains(std::string_view str) const {
    return LoggedText().find(str) != std::string::npos;
  }

  void ClearLog() { stream_logger_.stream.str({}); }

  template <typename T>
  std::string ToStringViaLogging(const T& value) {
    LOG_CRITICAL() << value;
    std::string result = LoggedText();
    ClearLog();
    return result;
  }

  std::string GetStreamString() const { return stream_logger_.stream.str(); }

  std::size_t GetRecordsCount() const {
    auto str = GetStreamString();
    return std::count(str.begin(), str.end(), '\n');
  }

  std::shared_ptr<logging::impl::TpLogger> GetStreamLogger() const {
    return stream_logger_.logger;
  }

 private:
  const logging::Format format_;
  const StringStreamLogger stream_logger_;
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
