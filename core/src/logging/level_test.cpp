#include <userver/utest/utest.hpp>

#include <logging/dynamic_debug.hpp>
#include <logging/logging_test.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

TEST_F(LoggingTest, DynamicDebug) {
  RunInCoro([&] {
    logging::SetDefaultLoggerLevel(logging::Level::kNone);

    LOG_INFO() << "before";

    std::string location = fmt::format("{}:{}", USERVER_FILEPATH, 19);
    logging::SetDynamicDebugLog(location, true);

    LOG_INFO() << "123";

    logging::SetDynamicDebugLog(location, false);

    LOG_INFO() << "after";

    logging::LogFlush();
    const auto log_contents = sstream.str();
    EXPECT_NE(log_contents, "");
    EXPECT_EQ(log_contents.find("text=before"), std::string::npos);
    EXPECT_NE(log_contents.find("text=123"), std::string::npos);
    EXPECT_EQ(log_contents.find("text=after"), std::string::npos);

    logging::SetDefaultLoggerLevel(logging::Level::kInfo);
  });
}

USERVER_NAMESPACE_END
