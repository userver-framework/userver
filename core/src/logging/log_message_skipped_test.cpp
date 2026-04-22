// This file tests USERVER_FEATURE_ERASE_LOG_WITH_LEVEL macro

#include <logging/dynamic_debug.hpp>
#include <logging/logging_test.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

#if USERVER_FEATURE_ERASE_LOG_WITH_LEVEL > 3
#define USERVER_IMPL_TEST_CONDITIONAL_DISABLE(X) X
#else
#define USERVER_IMPL_TEST_CONDITIONAL_DISABLE(X) DISABLED_##X
#endif

// NOLINTNEXTLINE(google-readability-avoid-underscore-in-googletest-name)
TEST_F(LoggingTest, USERVER_IMPL_TEST_CONDITIONAL_DISABLE(SkippedByMacro)) {
    const auto logger_data = MakeNamedStreamLogger("other-logger", logging::Format::kTskv);
    const auto& logger = logger_data.logger;

    SetDefaultLoggerLevel(logging::Level::kTrace);
    logging::SetLoggerLevel(*logger, logging::Level::kTrace);

#line 10000
    LOG_TRACE_TO(*logger) << "test";
    LOG_DEBUG_TO(*logger) << "test";
    LOG_INFO_TO(*logger) << "test";
    LOG_WARNING_TO(*logger) << "test";
    LOG_ERROR_TO(*logger) << "test";

    LOG_TRACE_TO(*logger, "test");
    LOG_DEBUG_TO(*logger, "test");
    LOG_INFO_TO(*logger, "test");
    LOG_WARNING_TO(*logger, "test");
    LOG_ERROR_TO(*logger, "test");

    LOG_LIMITED_TRACE_TO(*logger) << "test";
    LOG_LIMITED_DEBUG_TO(*logger) << "test";
    LOG_LIMITED_INFO_TO(*logger) << "test";
    LOG_LIMITED_WARNING_TO(*logger) << "test";
    LOG_LIMITED_ERROR_TO(*logger) << "test";

    LOG_LIMITED_TRACE_TO(*logger, "test");
    LOG_LIMITED_DEBUG_TO(*logger, "test");
    LOG_LIMITED_INFO_TO(*logger, "test");
    LOG_LIMITED_WARNING_TO(*logger, "test");
    LOG_LIMITED_ERROR_TO(*logger, "test");

    ////////////////////////////

    LOG_TRACE() << "test";
    LOG_DEBUG() << "test";
    LOG_INFO() << "test";
    LOG_WARNING() << "test";
    LOG_ERROR() << "test";

    LOG_TRACE("test");
    LOG_DEBUG("test");
    LOG_INFO("test");
    LOG_WARNING("test");
    LOG_ERROR("test");

    LOG_LIMITED_TRACE() << "test";
    LOG_LIMITED_DEBUG() << "test";
    LOG_LIMITED_INFO() << "test";
    LOG_LIMITED_WARNING() << "test";
    LOG_LIMITED_ERROR() << "test";

    LOG_LIMITED_TRACE("test");
    LOG_LIMITED_DEBUG("test");
    LOG_LIMITED_INFO("test");
    LOG_LIMITED_WARNING("test");
    LOG_LIMITED_ERROR("test");

    logging::LogFlush(*logger);

    EXPECT_TRUE(logger_data.stream.str().empty());
    EXPECT_TRUE(GetStreamString().empty());

    constexpr int kFirstLogLine = 10000;
    const std::string filename{USERVER_FILEPATH};
    for (int i = 0; i < 100; ++i) {
        // Make sure that no mentions of logs remain in binary, no StaticLogEntry
        EXPECT_THROW(logging::AddDynamicDebugLog(filename, kFirstLogLine + i), std::exception);
    }
}

USERVER_NAMESPACE_END
