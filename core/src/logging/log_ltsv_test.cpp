#include <gtest/gtest.h>

#include <logging/logging_test.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/log_helper_fwd.hpp>

USERVER_NAMESPACE_BEGIN

TEST_F(LoggingLtsvTest, Basic) {
  constexpr auto kLtsvTextToLog = "This is the LTSV text to log";
  LOG_INFO() << kLtsvTextToLog;

  logging::LogFlush();
  const auto log_contents = GetStreamString();

  EXPECT_EQ(LoggedText(), kLtsvTextToLog);

  auto str = GetStreamString();
  EXPECT_NE(str.find("\tmodule:"), std::string::npos) << str;
  EXPECT_NE(str.find("timestamp:"), std::string::npos) << str;
  EXPECT_NE(str.find("\tthread_id:"), std::string::npos) << str;
  EXPECT_NE(str.find("\tlevel:"), std::string::npos) << str;
}

USERVER_NAMESPACE_END
