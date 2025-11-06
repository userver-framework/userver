#include <unordered_set>

#include <engine/task/task_processor.hpp>
#include <userver/compiler/thread_local.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_base.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// Using implementation from scratch as the core implementation may be turned into noop under TSAN
__attribute__((noinline)) auto GetThreadIdSafe() {
    // Implementation note: this 'asm' and 'noinline' prevent the compiler
    // from caching the TLS address across a coroutine context switch.
    // The general approach is taken from:
    // https://github.com/qemu/qemu/blob/stable-8.2/include/qemu/coroutine-tls.h#L153

    auto id = std::this_thread::get_id();
    auto* ptr = &id;
    // NOLINTNEXTLINE(hicpp-no-assembler)
    asm volatile("" : "+rm"(ptr));
    return *ptr;
}

}  // namespace

TEST(TaskQueueTSan, TheSameThreadAfterYield) {
    engine::TaskProcessorPoolsConfig config{};
    config.queue_type = engine::TaskQueueType::kTSanTaskQueue;

    static constexpr std::size_t kWorkerThreads = 16;
    engine::RunStandalone(kWorkerThreads, config, []() {
        const auto payload = []() {
            const auto id_initial = GetThreadIdSafe();

            engine::Yield();
            EXPECT_EQ(id_initial, GetThreadIdSafe());

            for (int i = 10; i < 16; ++i) {
                engine::SleepFor(std::chrono::milliseconds(i));
                EXPECT_EQ(id_initial, GetThreadIdSafe());
            }
        };

        constexpr std::size_t kAsyncTasksTourtureCount = kWorkerThreads * 1000;
        std::vector<engine::Task> tasks;
        tasks.reserve(kAsyncTasksTourtureCount);
        for (std::size_t i = 0; i < kAsyncTasksTourtureCount; ++i) {
            tasks.emplace_back(engine::AsyncNoSpan(payload));
        }
    });
}

TEST(TaskQueueTSan, RoundRobin) {
    engine::TaskProcessorPoolsConfig config{};
    config.queue_type = engine::TaskQueueType::kTSanTaskQueue;

    static constexpr std::size_t kWorkerThreads = 16;
    engine::RunStandalone(kWorkerThreads, config, []() {
        std::unordered_set<std::thread::id> ids;
        const auto payload = [&ids]() { ids.insert(GetThreadIdSafe()); };

        for (std::size_t i = 0; i < kWorkerThreads; ++i) {
            engine::AsyncNoSpan(payload).Get();
        }

        EXPECT_EQ(ids.size(), kWorkerThreads);
    });
}

USERVER_NAMESPACE_END
