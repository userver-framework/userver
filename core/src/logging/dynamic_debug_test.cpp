#include <userver/utest/utest.hpp>

#include <logging/dynamic_debug.hpp>
#include <logging/logging_test.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

TEST_F(LoggingTest, DynamicDebugBasic) {
  logging::SetDefaultLoggerLevel(logging::Level::kNone);

  LOG_INFO() << "before";

  const std::string location = USERVER_FILEPATH;
  logging::AddDynamicDebugLog(location, 17);

  LOG_INFO() << "123";

  logging::RemoveDynamicDebugLog(location, 17);

  LOG_INFO() << "after";

  EXPECT_FALSE(LoggedTextContains("before"));
  EXPECT_TRUE(LoggedTextContains("123"));
  EXPECT_FALSE(LoggedTextContains("after"));

  logging::SetDefaultLoggerLevel(logging::Level::kInfo);
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

  EXPECT_FALSE(LoggedTextContains("before"));
  EXPECT_TRUE(LoggedTextContains("123"));
  EXPECT_TRUE(LoggedTextContains("456"));
  EXPECT_FALSE(LoggedTextContains("after"));

  logging::SetDefaultLoggerLevel(logging::Level::kInfo);
}

TEST_F(LoggingTest, DynamicDebugAnyLineRemove) {
  logging::SetDefaultLoggerLevel(logging::Level::kNone);

  LOG_INFO() << "before";

  const std::string location = USERVER_FILEPATH;
  logging::AddDynamicDebugLog(location, 63);
  logging::AddDynamicDebugLog(location, 64);
  logging::RemoveDynamicDebugLog(location, logging::kAnyLine);

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
