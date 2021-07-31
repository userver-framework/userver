#include <gtest/gtest.h>

#include <logging/logging_test.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/log_helper_fwd.hpp>

TEST_F(LoggingTest, SwitchToTraceWorks) {
  LOG_TRACE() << "test";
  ::logging::SetDefaultLoggerLevel(::logging::Level::kTrace);
  LOG_TRACE() << "test";
  ::logging::SetDefaultLoggerLevel(::logging::Level::kInfo);
  LOG_TRACE() << "test";
  ::logging::SetDefaultLoggerLevel(::logging::Level::kTrace);
  LOG_TRACE() << "test";
  ::logging::SetDefaultLoggerLevel(::logging::Level::kInfo);

  logging::LogFlush();
  const auto log_contents = sstream.str();
  size_t pos = 0;
  size_t entries = 0;
  while ((pos = log_contents.find("text=test", pos)) != std::string::npos) {
    ++entries;
    ++pos;
  }
  EXPECT_EQ(2, entries);
}

TEST_F(LoggingTest, ChronoDuration) {
  using namespace std::literals::chrono_literals;

  EXPECT_EQ("7ns", ToStringViaLogging(std::chrono::nanoseconds{7}));
  EXPECT_EQ("7us", ToStringViaLogging(std::chrono::microseconds{7}));
  EXPECT_EQ("7ms", ToStringViaLogging(std::chrono::milliseconds{7}));
  EXPECT_EQ("-7s", ToStringViaLogging(std::chrono::seconds{-7}));
  EXPECT_EQ("7min", ToStringViaLogging(std::chrono::minutes{7}));
  EXPECT_EQ("7h", ToStringViaLogging(std::chrono::hours{7}));
}

TEST_F(LoggingTest, Boolean) {
  EXPECT_EQ("false", ToStringViaLogging(false));
  EXPECT_EQ("true", ToStringViaLogging(true));
}

TEST_F(LoggingTest, DocsData) {
  /// [Sample logging usage]
  LOG_DEBUG() << "Some debug info, not logged by default in production";
  LOG_INFO() << "This is informational message";
  LOG_WARNING() << "Something strange happened";
  LOG_ERROR() << "This is unbelievable, fix me, please!";
  /// [Sample logging usage]

  bool flag = true;
  /// [Example set custom logging usage]
  logging::Level level = flag ? logging::Level::kDebug : logging::Level::kInfo;
  LOG(level) << "some text";
  /// [Example set custom logging usage]
}
