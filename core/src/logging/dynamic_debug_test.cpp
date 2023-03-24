#include <userver/utest/utest.hpp>

#include <gmock/gmock.h>

#include <logging/dynamic_debug.hpp>
#include <logging/logging_test.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

TEST_F(LoggingTest, DynamicDebugEnable) {
  logging::SetDefaultLoggerLevel(logging::Level::kNone);

  LOG_INFO() << "before";

  const std::string location = USERVER_FILEPATH;
  logging::AddDynamicDebugLog(location, 10001);

#line 10001
  LOG_INFO() << "123";

  logging::RemoveDynamicDebugLog(location, 10001);

  LOG_INFO() << "after";

  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("before")));
  EXPECT_THAT(GetStreamString(), testing::HasSubstr("123"));
  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("after")));

  logging::SetDefaultLoggerLevel(logging::Level::kInfo);
}

TEST_F(LoggingTest, DynamicDebugDisable) {
  logging::SetDefaultLoggerLevel(logging::Level::kInfo);

  const std::string location = USERVER_FILEPATH;
  logging::AddDynamicDebugLog(location, 10002,
                              logging::EntryState::kForceDisabled);

#line 10002
  LOG_INFO() << "here";

  logging::RemoveDynamicDebugLog(location, 10002);

  EXPECT_FALSE(LoggedTextContains("here"));
}

TEST_F(LoggingTest, DynamicDebugAnyLine) {
  logging::SetDefaultLoggerLevel(logging::Level::kNone);

  LOG_INFO() << "before";

  const std::string location = USERVER_FILEPATH;
  logging::AddDynamicDebugLog(location, logging::kAnyLine);

  LOG_INFO() << "123";
  LOG_INFO() << "456";

  logging::RemoveDynamicDebugLog(location, logging::kAnyLine);

  LOG_INFO() << "after";

  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("before")));
  EXPECT_THAT(GetStreamString(), testing::HasSubstr("123"));
  EXPECT_THAT(GetStreamString(), testing::HasSubstr("456"));
  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("after")));

  logging::SetDefaultLoggerLevel(logging::Level::kInfo);
}

TEST_F(LoggingTest, DynamicDebugAnyLineRemove) {
  logging::SetDefaultLoggerLevel(logging::Level::kNone);

  LOG_INFO() << "before";

  const std::string location = USERVER_FILEPATH;
  logging::AddDynamicDebugLog(location, 20001);
  logging::AddDynamicDebugLog(location, 20002);
  logging::RemoveDynamicDebugLog(location, logging::kAnyLine);

#line 20001
  LOG_INFO() << "123";
  LOG_INFO() << "456";

  const std::string kBadPath = "Non existing path (*&#(R&!(!@(*)*#&)@#$!";
  UEXPECT_THROW_MSG(logging::AddDynamicDebugLog(kBadPath, 1),
                    std::runtime_error, kBadPath);

  const int kBadLine{98888988};
  UEXPECT_THROW_MSG(logging::AddDynamicDebugLog(location, kBadLine),
                    std::runtime_error, "98888988");

  EXPECT_FALSE(LoggedTextContains("before"));
  EXPECT_FALSE(LoggedTextContains("123"));
  EXPECT_FALSE(LoggedTextContains("456"));
  EXPECT_FALSE(LoggedTextContains("after"));

  logging::SetDefaultLoggerLevel(logging::Level::kInfo);
}

USERVER_NAMESPACE_END
