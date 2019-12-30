#include <gtest/gtest.h>

#include <logging/log.hpp>
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
