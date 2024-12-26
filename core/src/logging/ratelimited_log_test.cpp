#include <userver/utest/utest.hpp>

#include <gmock/gmock.h>

#include <logging/logging_test.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

TEST_F(LoggingTest, RatelimitedForce) {
    SetDefaultLoggerLevel(logging::Level::kDebug);
    for (size_t i = 0; i < 100; i++) {
        LOG_LIMITED_INFO() << "smth";
        auto str = GetStreamString();

        // check that the log is not skipped
        EXPECT_THAT(str, testing::HasSubstr("smth")) << str;

        ClearLog();
    }
}

USERVER_NAMESPACE_END
