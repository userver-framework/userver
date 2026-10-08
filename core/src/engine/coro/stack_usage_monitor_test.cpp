#include <userver/utest/utest.hpp>

#include <algorithm>
#include <array>
#include <csignal>
#include <thread>
#include <vector>

#include <gmock/gmock.h>

#include <engine/coro/stack_usage_monitor.hpp>
#include <engine/task/task_context.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/rand.hpp>

#include <logging/logging_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class StackConsumingScope final {
public:
    StackConsumingScope() {
        utils::WithDefaultRandom([this](auto& rnd) {
            for (auto& value : values_) {
                value = rnd();
            }
        });
    }

    std::uint32_t Get() const noexcept { return values_[utils::RandRange(kValuesCount)]; }

private:
    static constexpr std::size_t kValuesCount = 256;
    std::array<std::uint32_t, kValuesCount> values_{};
};

__attribute__((noinline)) std::uint64_t SelfLimitingRecursiveFunction() {
    const StackConsumingScope scope{};

    const auto stack_size = engine::current_task::GetStackSize();
    const auto stack_usage = engine::coro::GetCurrentTaskStackUsageBytes();
    if (stack_usage >= stack_size * 0.85) {
        return 1;
    }

    return scope.Get() + SelfLimitingRecursiveFunction();
}

}  // namespace

#if defined(__linux__) && defined(__x86_64__)
TEST(StackUsageMonitor, PreservesExistingAlternateSignalStack) {
    if (!engine::coro::StackUsageMonitor::DebugCanUseUserfaultfd()) {
        GTEST_SKIP() << "userfaultfd is unavailable";
    }

    constexpr std::size_t kStackSize = 64 * 1024;
    engine::coro::StackUsageMonitor monitor{kStackSize};
    std::thread thread([&monitor] {
        std::vector<char> stack(kStackSize);
        stack_t original{};
        stack_t installed{};
        installed.ss_sp = stack.data();
        installed.ss_size = stack.size();
        ASSERT_EQ(sigaltstack(&installed, &original), 0);

        monitor.RegisterThread();

        stack_t actual{};
        EXPECT_EQ(sigaltstack(nullptr, &actual), 0);
        EXPECT_EQ(actual.ss_sp, installed.ss_sp);
        EXPECT_EQ(actual.ss_size, installed.ss_size);
        EXPECT_EQ(sigaltstack(&original, nullptr), 0);
    });
    thread.join();
}
#endif

class StackUsageMonitorTest : public LoggingTest {};

UTEST_F(StackUsageMonitorTest, BacktraceLogging) {
    if (!engine::coro::StackUsageMonitor::DebugCanUseUserfaultfd()) {
        GTEST_SKIP()
            << "Target platform doesn't have/support userspace "
               "page-faulting via userfaultfd(2)";
    }

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
    GTEST_SKIP() << "This test doesn't reliably work with ASAN";
#endif
#endif

    SetDefaultLoggerLevel(logging::Level::kWarning);
    EXPECT_NE(1, SelfLimitingRecursiveFunction());
    engine::Yield();

    logging::LogFlush();

    const auto logged_string = GetStreamString();
    // Where the backtrace is logged
    EXPECT_THAT(logged_string, testing::HasSubstr("module=AccountStackUsage"));
    // What is logged: stack usage
    EXPECT_THAT(logged_string, testing::HasSubstr("Coroutine is using approximately"));
    // What is logged: where the backtrace is created
    EXPECT_THAT(logged_string, testing::HasSubstr("StackUsageSignalHandler"));
    // What is logged: backtrace is reasonable
    EXPECT_THAT(logged_string, testing::HasSubstr("SelfLimitingRecursiveFunction"));
    // Name of the test
    EXPECT_THAT(logged_string, testing::HasSubstr("StackUsageMonitorTest_BacktraceLogging_Utest"));
    // Userver stacktrace-caching machinery adds this
    EXPECT_THAT(logged_string, testing::HasSubstr("[start of coroutine]"));
}

USERVER_NAMESPACE_END
