#include <gtest/gtest.h>

#include <logging/log.hpp>
#include <logging/log_helper_fwd.hpp>
#include <logging/logging_test.hpp>

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
