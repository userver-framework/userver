#include <unordered_set>

#include <engine/task/task_processor.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/get_all.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_base.hpp>
#include <userver/utest/utest.hpp>

#include <engine/task/thread_id_test.hpp>

USERVER_NAMESPACE_BEGIN

TEST(TaskQueuePullPin, TheSameThreadAfterYield) {
    engine::TaskProcessorPoolsConfig config{};
    config.queue_type = engine::TaskQueueType::kPullPinTaskQueue;

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
        std::vector<engine::TaskWithResult<void>> tasks;
        tasks.reserve(kAsyncTasksTourtureCount);
        for (std::size_t i = 0; i < kAsyncTasksTourtureCount; ++i) {
            tasks.emplace_back(engine::AsyncNoSpan(payload));
        }

        engine::GetAll(tasks);
    });
}

TEST(TaskQueuePullPin, TheSameThreadAfterLockUnlock) {
    engine::TaskProcessorPoolsConfig config{};
    config.queue_type = engine::TaskQueueType::kPullPinTaskQueue;

    static constexpr std::size_t kWorkerThreads = 16;
    engine::RunStandalone(kWorkerThreads, config, []() {
        engine::Mutex mutexes[4];

        const auto payload = [&mutexes]() {
            const auto id_initial = test::GetThreadIdSafe();

            {
                const std::scoped_lock lock{mutexes[0], mutexes[1], mutexes[2], mutexes[3]};
                engine::SleepFor(std::chrono::milliseconds(1));
                EXPECT_EQ(id_initial, test::GetThreadIdSafe());
            }
            EXPECT_EQ(id_initial, test::GetThreadIdSafe());
        };

        constexpr std::size_t kAsyncTasksTourtureCount = kWorkerThreads * 10;
        std::vector<engine::TaskWithResult<void>> tasks;
        tasks.reserve(kAsyncTasksTourtureCount);
        for (std::size_t i = 0; i < kAsyncTasksTourtureCount; ++i) {
            tasks.emplace_back(engine::AsyncNoSpan(payload));
        }

        engine::GetAll(tasks);
    });
}

USERVER_NAMESPACE_END
