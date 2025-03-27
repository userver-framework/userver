#include <gtest/gtest.h>

#include <gmock/gmock.h>

#include <logging/logging_test.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

TEST_F(LoggingLtsvTest, Basic) {
    constexpr auto kLtsvTextToLog = "This is the LTSV text to log";
    LOG_INFO() << kLtsvTextToLog;

    logging::LogFlush();
    const auto log_contents = GetStreamString();

    EXPECT_EQ(LoggedText(), kLtsvTextToLog);

    const auto str = GetStreamString();
    EXPECT_THAT(str, testing::HasSubstr("\tmodule:"));
    EXPECT_THAT(str, testing::HasSubstr("timestamp:"));
    EXPECT_THAT(str, testing::HasSubstr("\tlevel:"));
}

USERVER_NAMESPACE_END
