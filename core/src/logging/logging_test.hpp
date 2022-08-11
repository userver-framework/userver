#include <gtest/gtest.h>

#include <logging/spdlog.hpp>

#include <logging/config.hpp>
#include <logging/logger_with_info.hpp>
#include <logging/spdlog_helpers.hpp>

#include <spdlog/sinks/ostream_sink.h>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

inline logging::LoggerPtr MakeNamedStreamLogger(const std::string& logger_name,
                                                std::ostream& stream,
                                                logging::Format format) {
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_st>(stream);
  return std::make_shared<logging::impl::LoggerWithInfo>(
      format, std::shared_ptr<spdlog::details::thread_pool>{},
      utils::MakeSharedRef<spdlog::logger>(logger_name, sink));
}

class LoggingTestBase : public ::testing::Test {
 protected:
  LoggingTestBase(logging::Format format, std::string_view text_key)
      : format_(format), text_key_(text_key) {}

  void SetUp() override {
    old_ = logging::SetDefaultLogger(MakeStreamLogger(sstream));

    // Discard logs from SetDefaultLogger
    logging::LogFlush();
    ClearLog();
  }

  void TearDown() override {
    if (old_) {
      logging::SetDefaultLogger(old_);
      old_.reset();
    }
  }

  logging::LoggerPtr MakeStreamLogger(std::ostream& stream) const {
    std::ostringstream os;
    os << this;
    auto logger = MakeNamedStreamLogger(os.str(), stream, format_);
    logger->ptr->set_pattern(logging::GetSpdlogPattern(format_));
    return logger;
  }

  std::ostringstream sstream;

  std::string LoggedText() const {
    logging::LogFlush();
    const std::string str = sstream.str();

    const auto text_begin = str.find(text_key_);
    const std::string from_text =
        text_begin == std::string::npos
            ? str
            : str.substr(text_begin + text_key_.length());

    const auto text_end = from_text.rfind('\n');
    return text_end == std::string::npos ? from_text
                                         : from_text.substr(0, text_end);
  }

  bool LoggedTextContains(const std::string& str) const {
    return LoggedText().find(str) != std::string::npos;
  }

  void ClearLog() { sstream.str({}); }

  template <typename T>
  std::string ToStringViaLogging(const T& value) {
    LOG_CRITICAL() << value;
    const std::string result = LoggedText();
    ClearLog();
    return result;
  }

 private:
  const logging::Format format_;
  const std::string_view text_key_;
  logging::LoggerPtr old_;
};

class LoggingTest : public LoggingTestBase {
 protected:
  LoggingTest() : LoggingTestBase(logging::Format::kTskv, "text=") {}
};

class LoggingLtsvTest : public LoggingTestBase {
 protected:
  LoggingLtsvTest() : LoggingTestBase(logging::Format::kLtsv, "text:") {}
};

USERVER_NAMESPACE_END
