#pragma once

#include <gtest/gtest.h>

#include <logging/spdlog.hpp>

#include <logging/config.hpp>
#include <logging/logger_with_info.hpp>
#include <logging/spdlog_helpers.hpp>
#include <logging/unix_socket_sink.hpp>

#include <spdlog/sinks/ostream_sink.h>

#include <userver/engine/condition_variable.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/format.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/async.hpp>
#include <utils/check_syscall.hpp>

#include <sys/socket.h>
#include <sys/un.h>

USERVER_NAMESPACE_BEGIN

inline logging::LoggerPtr MakeLoggerFromSink(
    const std::string& logger_name,
    std::shared_ptr<spdlog::sinks::sink> sink_ptr, logging::Format format) {
  return std::make_shared<logging::impl::LoggerWithInfo>(
      format, std::shared_ptr<spdlog::details::thread_pool>{},
      utils::MakeSharedRef<spdlog::logger>(logger_name, sink_ptr));
}

inline logging::LoggerPtr MakeNamedStreamLogger(const std::string& logger_name,
                                                std::ostream& stream,
                                                logging::Format format) {
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_st>(stream);
  return MakeLoggerFromSink(logger_name, sink, format);
}

inline logging::LoggerPtr MakeSocketLogger(const std::string& logger_name,
                                           std::string& filename,
                                           logging::Format format) {
  auto sink = std::make_shared<logging::SocketSinkST>(filename);
  return MakeLoggerFromSink(logger_name, sink, format);
}

class SocketLoggingTest : public ::testing::Test {
 public:
  static constexpr char socket_file[] = "socket";
  static constexpr size_t buffer_size = 1024;

  std::string LoggedText() const {
    logging::LogFlush();
    std::string str(buffer_);
    std::string text_key = "text=";

    const auto text_begin = str.find(text_key);
    const std::string from_text =
        text_begin == std::string::npos
            ? str
            : str.substr(text_begin + text_key.length());

    const auto text_end = from_text.find('\t');
    return text_end == std::string::npos ? from_text
                                         : from_text.substr(0, text_end);
  }

  void Server(engine::ConditionVariable& cv) {
    engine::io::Socket connection = socket_.Accept({});

    starts_.store(true);
    cv.NotifyOne();
    while (!engine::current_task::ShouldCancel()) {
      auto res = connection.RecvSome(buffer_ + bytes_read_,
                                     buffer_size - bytes_read_, {});

      bytes_read_ += static_cast<size_t>(res);
      UASSERT(bytes_read_ < buffer_size);
      cv.NotifyOne();
    }
  }

  bool IsStarts() const { return starts_.load(); }
  bool IsRead() const { return bytes_read_.load() != 0; }

 protected:
  SocketLoggingTest()
      : socket_(engine::io::AddrDomain::kUnix,
                engine::io::SocketType::kStream) {
    unlink(socket_file);

    struct sockaddr_un addr {};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_file, std::strlen(socket_file));

    socket_.Bind(engine::io::Sockaddr(static_cast<const void*>(&addr)));

    socket_.Listen();
  }

  void SetUp() override {
    static std::string logger_name = "socket_logger";
    std::string filename(socket_file);
    auto logger =
        MakeSocketLogger(logger_name, filename, logging::Format::kTskv);
    logger->ptr->set_pattern(logging::GetSpdlogPattern(logging::Format::kTskv));
    logger->ptr->set_level(spdlog::level::level_enum::err);
    old_ = logging::SetDefaultLogger(logger);

    logging::LogFlush();
    ClearLog();
  }

  void TearDown() override {
    if (old_) {
      // Discard logs from SetDefaultLogger
      logging::SetDefaultLoggerLevel(logging::Level::kNone);
      logging::LogFlush();

      logging::SetDefaultLogger(old_);
      old_.reset();
    }
  }

 private:
  void ClearLog() { bytes_read_ = 0; }

  engine::io::Socket socket_;
  std::atomic<bool> starts_;

  char buffer_[buffer_size]{};
  std::atomic<size_t> bytes_read_{0};

  logging::LoggerPtr old_;
};

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
      // Discard logs from SetDefaultLogger
      logging::SetDefaultLoggerLevel(logging::Level::kNone);
      logging::LogFlush();

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
    std::string result = LoggedText();
    ClearLog();
    return result;
  }

  std::string GetStreamString() const { return sstream.str(); }

 private:
  const logging::Format format_;
  const std::string_view text_key_;
  logging::LoggerPtr old_;

  std::ostringstream sstream;
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
