#include <atomic>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/get_all.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

TEST(FastWorkStealingQueue, Smoke) {
    engine::TaskProcessorPoolsConfig config{};
    config.queue_type = engine::TaskQueueType::kWorkStealingTaskQueue;

    static constexpr std::size_t kWorkerThreads = 4;
    engine::RunStandalone(kWorkerThreads, config, []() {
        std::atomic<std::size_t> counter{0};

        constexpr std::size_t kTasks = kWorkerThreads * 100;
        std::vector<engine::TaskWithResult<void>> tasks;
        tasks.reserve(kTasks);
        for (std::size_t i = 0; i < kTasks; ++i) {
            tasks.emplace_back(engine::AsyncNoTracing([&counter] { counter.fetch_add(1); }));
        }

        engine::GetAll(tasks);
        EXPECT_EQ(counter.load(), kTasks);
    });
}

TEST(FastWorkStealingQueue, NestedSpawn) {
    engine::TaskProcessorPoolsConfig config{};
    config.queue_type = engine::TaskQueueType::kWorkStealingTaskQueue;

    engine::RunStandalone(4, config, []() {
        std::atomic<std::size_t> counter{0};

        auto payload = [&counter]() {
            for (std::size_t i = 0; i < 10; ++i) {
                engine::AsyncNoTracing([&counter] { counter.fetch_add(1); }).Wait();
                engine::Yield();
            }
        };

        std::vector<engine::TaskWithResult<void>> tasks;
        static constexpr std::size_t kTaskCount = 4;
        tasks.reserve(kTaskCount);
        for (std::size_t i = 0; i < kTaskCount; ++i) {
            tasks.emplace_back(engine::AsyncNoTracing(payload));
        }
        engine::GetAll(tasks);

        EXPECT_EQ(counter.load(), 40U);
    });
}

USERVER_NAMESPACE_END
