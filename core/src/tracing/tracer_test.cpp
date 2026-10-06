#include <userver/tracing/tracer.hpp>

#include <array>
#include <atomic>
#include <barrier>
#include <cstddef>
#include <optional>
#include <thread>
#include <vector>

#include <logging/logging_test.hpp>

#include <userver/logging/level.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

TEST(TracerConfiguration, SpanLogLevelRoundTripAndReset) {
    constexpr std::array kLevels{
        logging::Level::kTrace,
        logging::Level::kDebug,
        logging::Level::kInfo,
        logging::Level::kWarning,
        logging::Level::kError,
        logging::Level::kCritical,
        logging::Level::kNone,
    };
    const TracingStateGuard tracing_state_guard;

    EXPECT_EQ(GetSpanLogLevel(), std::nullopt);
    for (const auto level : kLevels) {
        SetSpanLogLevel(level);
        EXPECT_EQ(GetSpanLogLevel(), level);
        SetSpanLogLevel(std::nullopt);
        EXPECT_EQ(GetSpanLogLevel(), std::nullopt);
    }
}

TEST(TracerConfiguration, ConcurrentSpanLogLevelAccessWithReset) {
    constexpr std::size_t kThreadCount = 4;
    constexpr std::size_t kIterationCount = 10'000;
    constexpr std::array<std::optional<logging::Level>, 3>
        kLevels{std::nullopt, logging::Level::kInfo, logging::Level::kNone};
    const TracingStateGuard tracing_state_guard;
    std::barrier start{kThreadCount};
    std::atomic<bool> has_unexpected_level{false};
    std::vector<std::jthread> threads;
    threads.reserve(kThreadCount);

    for (std::size_t thread_index = 0; thread_index < kThreadCount; ++thread_index) {
        threads.emplace_back([&, thread_index] {
            start.arrive_and_wait();
            for (std::size_t iteration = 0; iteration < kIterationCount; ++iteration) {
                SetSpanLogLevel(kLevels[(thread_index + iteration) % kLevels.size()]);
                const auto level = GetSpanLogLevel();
                if (level != std::nullopt && level != logging::Level::kInfo && level != logging::Level::kNone) {
                    has_unexpected_level.store(true);
                }
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_FALSE(has_unexpected_level.load());
}

class TracerLogLevel : public LoggingTest {};

UTEST_F(TracerLogLevel, ResetRestoresCurrentDefaultLoggerLevel) {
    const logging::DefaultLoggerLevelScope default_logger_level{logging::Level::kWarning};
    const auto span = Span::MakeRootSpan("span-log-level-reset");
    EXPECT_FALSE(span.ShouldLogDefault());

    SetSpanLogLevel(logging::Level::kInfo);
    EXPECT_TRUE(span.ShouldLogDefault());

    SetSpanLogLevel(std::nullopt);
    EXPECT_FALSE(span.ShouldLogDefault());

    const logging::DefaultLoggerLevelScope updated_default_logger_level{logging::Level::kInfo};
    EXPECT_TRUE(span.ShouldLogDefault());

    SetSpanLogLevel(logging::Level::kNone);
    EXPECT_FALSE(span.ShouldLogDefault());

    SetSpanLogLevel(std::nullopt);
    EXPECT_TRUE(span.ShouldLogDefault());
}

}  // namespace tracing

USERVER_NAMESPACE_END
