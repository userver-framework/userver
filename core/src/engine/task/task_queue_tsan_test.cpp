#include <unordered_set>

#include <engine/task/task_processor.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_base.hpp>
#include <userver/utest/utest.hpp>

#include <engine/task/thread_id_test.hpp>

USERVER_NAMESPACE_BEGIN

TEST(TaskQueueTSan, TheSameThreadAfterYield) {
    engine::TaskProcessorPoolsConfig config{};
    config.queue_type = engine::TaskQueueType::kTSanTaskQueue;

    static constexpr std::size_t kWorkerThreads = 16;
    engine::RunStandalone(kWorkerThreads, config, []() {
        const auto payload = []() {
            const auto id_initial = test::GetThreadIdSafe();

            engine::Yield();
            EXPECT_EQ(id_initial, test::GetThreadIdSafe());

            for (int i = 10; i < 16; ++i) {
                engine::SleepFor(std::chrono::milliseconds(i));
                EXPECT_EQ(id_initial, test::GetThreadIdSafe());
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
        const auto payload = [&ids]() { ids.insert(test::GetThreadIdSafe()); };

        for (std::size_t i = 0; i < kWorkerThreads; ++i) {
            engine::AsyncNoSpan(payload).Get();
        }

        EXPECT_EQ(ids.size(), kWorkerThreads);
    });
}

USERVER_NAMESPACE_END
