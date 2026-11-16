#include <ugrpc/impl/event_engine/executor.hpp>

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/single_use_event.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/engine/task/inherited_variable.hpp>
#include <userver/utest/utest.hpp>

#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using ugrpc::impl::Executor;

bool IsExecutorThread(const Executor& executor) {
    return engine::current_task::IsTaskProcessorThread() &&
           &engine::current_task::GetTaskProcessor() == &executor.GetTaskProcessor();
}

engine::TaskInheritedVariable<int> producer_variable;

class BorrowedClosure final : public Executor::Closure {
public:
    explicit BorrowedClosure(std::function<void()> function)
        : function_(std::move(function))
    {}
    void Run() override { function_(); }

private:
    std::function<void()> function_;
};

class OnDestruction final {
public:
    explicit OnDestruction(std::function<void()> function)
        : function_(std::move(function))
    {}
    ~OnDestruction() { function_(); }

private:
    std::function<void()> function_;
};

}  // namespace

UTEST(GrpcEventEngineCallbackExecutor, BothOverloadsAreDeferredAndKeepTheirOwnership) {
    EXPECT_FALSE(Executor::IsInCallback());
    Executor executor{engine::current_task::GetTaskProcessor()};
    int calls = 0;
    int destroyed = 0;
    BorrowedClosure borrowed{[&] {
        EXPECT_TRUE(IsExecutorThread(executor));
        ++calls;
    }};
    executor.Run(&borrowed);
    executor.Run([&, capture = std::make_unique<OnDestruction>([&] {
                         EXPECT_TRUE(Executor::IsInCallback());
                         EXPECT_TRUE(IsExecutorThread(executor));
                         ++destroyed;
                     })] {
        EXPECT_TRUE(Executor::IsInCallback());
        EXPECT_TRUE(IsExecutorThread(executor));
        EXPECT_EQ(destroyed, 0);
        EXPECT_TRUE(engine::current_task::impl::IsCritical());
        ++calls;
    });
    EXPECT_EQ(calls, 0);
    executor.Quiesce();
    EXPECT_EQ(calls, 2);
    EXPECT_EQ(destroyed, 1);
    borrowed.Run();
    EXPECT_EQ(calls, 3);
    executor.Quiesce();
}

UTEST(GrpcEventEngineCallbackExecutor, RecursiveRunAndCaptureDestructionAreDrained) {
    Executor executor{engine::current_task::GetTaskProcessor()};
    int calls = 0;
    executor.Run([&, capture = std::make_unique<OnDestruction>([&] {
                         EXPECT_TRUE(IsExecutorThread(executor));
                         executor.Run([&] { ++calls; });
                     })] {
        executor.Run([&] {
            executor.Run([&] { ++calls; });
            ++calls;
        });
        EXPECT_EQ(calls, 0);
        ++calls;
    });
    executor.Quiesce();
    EXPECT_EQ(calls, 4);
}

UTEST(GrpcEventEngineCallbackExecutor, DoesNotInheritProducerContextOrCancellation) {
    Executor executor{engine::current_task::GetTaskProcessor()};
    producer_variable.Set(42);
    auto producer = engine::AsyncNoTracing([&] {
        producer_variable.Set(73);
        engine::current_task::SetDeadline(engine::Deadline::Passed());
        engine::current_task::RequestCancel();
        executor.Run([] {
            EXPECT_EQ(producer_variable.GetOptional(), nullptr);
            EXPECT_FALSE(engine::current_task::impl::GetDeadline().IsReachable());
            EXPECT_FALSE(engine::current_task::IsCancelRequested());
            engine::current_task::RequestCancel();
            EXPECT_FALSE(engine::current_task::ShouldCancel());
            engine::current_task::CancellationPoint();
        });
    });
    producer.Get();
    executor.Quiesce();
    EXPECT_EQ(producer_variable.Get(), 42);
    producer_variable.Erase();
}

UTEST_MT(GrpcEventEngineCallbackExecutor, ConcurrentForeignProducersAndWorkers, 4) {
    Executor executor{engine::current_task::GetTaskProcessor()};
    constexpr std::size_t kProducers = 4;
    constexpr std::size_t kCallbacks = 500;
    std::vector<std::atomic<int>> calls(kProducers * kCallbacks);
    std::vector<std::atomic<int>> destroyed(kProducers * kCallbacks);
    std::vector<engine::SingleUseEvent> finished(kProducers);
    std::vector<std::thread> producers;
    producers.reserve(kProducers);
    for (std::size_t p = 0; p < kProducers; ++p) {
        producers.emplace_back([&, p] {
            for (std::size_t i = 0; i < kCallbacks; ++i) {
                const auto index = p * kCallbacks + i;
                executor.Run([&, index, capture = std::make_unique<OnDestruction>([&, index] {
                                            EXPECT_TRUE(IsExecutorThread(executor));
                                            ++destroyed[index];
                                        })] {
                    EXPECT_TRUE(IsExecutorThread(executor));
                    ++calls[index];
                });
            }
            finished[p].Send();
        });
    }
    for (auto& event : finished) {
        event.WaitNonCancellable();
    }
    for (auto& producer : producers) {
        producer.join();
    }
    executor.Quiesce();
    for (std::size_t i = 0; i < calls.size(); ++i) {
        EXPECT_EQ(calls[i].load(), 1);
        EXPECT_EQ(destroyed[i].load(), 1);
    }
}

USERVER_NAMESPACE_END
