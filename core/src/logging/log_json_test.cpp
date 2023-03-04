#include <gtest/gtest.h>

#include <logging/logging_test.hpp>
#include <userver/logging/log.hpp>
#include <userver/formats/json/serialize.hpp>

USERVER_NAMESPACE_BEGIN

TEST_F(LoggingJsonTest, Basic) {
  constexpr auto kJsonTextToLog = "This is the JSON text to log";
  LOG_INFO() << kJsonTextToLog;

  EXPECT_EQ(LoggedText(), kJsonTextToLog);

  auto str = GetStreamString();
  EXPECT_NE(str.find(R"("timestamp":)"), std::string::npos) << str;
  EXPECT_NE(str.find(R"("level":)"), std::string::npos) << str;
  EXPECT_NE(str.find("module:"), std::string::npos) << str;
  EXPECT_NE(str.find("task_id:"), std::string::npos) << str;
  EXPECT_NE(str.find("thread_id:"), std::string::npos) << str;
  EXPECT_NE(str.find("text:"), std::string::npos) << str;

  ASSERT_NO_THROW(formats::json::FromString(str));
}

USERVER_NAMESPACE_END
