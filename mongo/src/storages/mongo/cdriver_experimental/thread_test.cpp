#include <storages/mongo/cdriver_experimental/thread.hpp>

#include <atomic>
#include <cerrno>
#include <deque>
#include <stdexcept>
#include <vector>

#include <userver/engine/deadline.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace cdriver = storages::mongo::impl::cdriver_experimental;

struct ThreadState {
    engine::SingleConsumerEvent started;
    engine::SingleConsumerEvent finish;
    std::atomic<bool> finished{false};
};

void* WaitForFinish(void* arg) {
    auto& state = *static_cast<ThreadState*>(arg);
    state.started.Send();
    EXPECT_TRUE(state.finish.WaitForEvent());
    state.finished = true;
    return nullptr;
}

struct BackendCondition {
    BackendCondition() {
        backend.mutex_init(&mutex);
        backend.cond_init(&cond);
    }
    ~BackendCondition() {
        backend.cond_destroy(&cond);
        backend.mutex_destroy(&mutex);
    }
    const bson_thread_backend_t& backend = cdriver::GetThreadBackend();
    int mutex{};
    int cond{};
};

}  // namespace

TEST(MongocGlobalLifecycle, MutexOutsideCoroutine) {
    ASSERT_FALSE(engine::current_task::IsTaskProcessorThread());
    const cdriver::MongocGlobalLifecycleScope scope;
    const auto& backend = cdriver::GetThreadBackend();
    int mutex{};
    backend.mutex_init(&mutex);
    backend.mutex_lock(&mutex);
    backend.mutex_unlock(&mutex);
    backend.mutex_destroy(&mutex);
}

TEST(MongocGlobalLifecycle, MutexSurvivesEngineRestart) {
    const auto& backend = cdriver::GetThreadBackend();
    int mutex{};
    {
        const cdriver::MongocGlobalLifecycleScope scope;
        backend.mutex_init(&mutex);
        backend.mutex_lock(&mutex);
        backend.mutex_unlock(&mutex);
    }
    for (int iteration = 0; iteration < 2; ++iteration) {
        engine::RunStandalone([&] {
            backend.mutex_lock(&mutex);
            backend.mutex_unlock(&mutex);
            void* thread{};
            ASSERT_EQ(backend.thread_create(&thread, [](void*) -> void* { return nullptr; }, nullptr), 0);
            EXPECT_EQ(backend.thread_join(&thread), 0);
        });
    }
    {
        const cdriver::MongocGlobalLifecycleScope scope;
        backend.mutex_lock(&mutex);
        backend.mutex_unlock(&mutex);
        backend.mutex_destroy(&mutex);
    }
}

UTEST_MT(MongocMutex, RegistryChurnWhileContended, 4) {
    constexpr int kTasks = 8;
    const auto& backend = cdriver::GetThreadBackend();
    int shared_mutex{};
    backend.mutex_init(&shared_mutex);
    int counter = 0;
    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(kTasks);
    for (int task = 0; task < kTasks; ++task) {
        tasks.push_back(utils::Async("mongo-registry-churn", [&] {
            std::vector<int> mutexes(512);
            std::vector<int> conditions(512);
            std::vector<int> shared_mutexes(512);
            for (int iteration = 0; iteration < 4; ++iteration) {
                for (std::size_t i = 0; i < mutexes.size(); ++i) {
                    backend.mutex_init(&mutexes[i]);
                    backend.cond_init(&conditions[i]);
                    backend.shared_mutex_init(&shared_mutexes[i]);
                }
                for (std::size_t i = 0; i < mutexes.size(); ++i) {
                    backend.mutex_lock(&shared_mutex);
                    ++counter;
                    // Force contenders to suspend while other registry entries change.
                    engine::Yield();
                    backend.mutex_unlock(&shared_mutex);
                    backend.mutex_lock(&mutexes[i]);
                    EXPECT_TRUE(backend.mutex_is_locked(&mutexes[i]));
                    backend.mutex_unlock(&mutexes[i]);
                    backend.shared_mutex_lock_shared(&shared_mutexes[i]);
                    backend.shared_mutex_unlock_shared(&shared_mutexes[i]);
                    backend.cond_signal(&conditions[i]);
                }
                for (std::size_t i = 0; i < mutexes.size(); ++i) {
                    backend.mutex_destroy(&mutexes[i]);
                    backend.cond_destroy(&conditions[i]);
                    backend.shared_mutex_destroy(&shared_mutexes[i]);
                }
                // The next iteration reuses exactly the same storage addresses.
            }
        }));
    }
    for (auto& task : tasks) {
        task.Get();
    }
    EXPECT_EQ(counter, kTasks * 4 * 512);
    backend.mutex_destroy(&shared_mutex);
}

UTEST_MT(MongocThread, ConcurrentCreateAndJoin, 4) {
    const auto& backend = cdriver::GetThreadBackend();
    constexpr int kTasks = 16;
    constexpr int kIterations = 16;
    std::atomic<int> completed{0};
    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(kTasks);
    for (int i = 0; i < kTasks; ++i) {
        tasks.push_back(utils::Async("create-and-join", [&] {
            for (int j = 0; j < kIterations; ++j) {
                void* thread{};
                ASSERT_EQ(
                    backend.thread_create(
                        &thread,
                        [](void* arg) -> void* {
                            ++*static_cast<std::atomic<int>*>(arg);
                            return nullptr;
                        },
                        &completed
                    ),
                    0
                );
                EXPECT_EQ(backend.thread_join(&thread), 0);
            }
        }));
    }
    for (auto& task : tasks) {
        task.Get();
    }
    EXPECT_EQ(completed, kTasks * kIterations);
}

UTEST(MongocThread, CallbackExceptionDoesNotEscapeJoin) {
    const auto& backend = cdriver::GetThreadBackend();
    void* thread{};
    ASSERT_EQ(
        backend.thread_create(&thread, [](void*) -> void* { throw std::runtime_error("mongo thread test"); }, nullptr),
        0
    );
    EXPECT_EQ(backend.thread_join(&thread), EFAULT);
    EXPECT_EQ(backend.thread_join(&thread), ESRCH);
}

UTEST(MongocThread, CreateAndJoin) {
    const auto& backend = cdriver::GetThreadBackend();
    ThreadState state;
    void* thread{};
    ASSERT_EQ(backend.thread_create(&thread, WaitForFinish, &state), 0);
    EXPECT_TRUE(state.started.WaitForEvent());
    state.finish.Send();
    EXPECT_EQ(backend.thread_join(&thread), 0);
    EXPECT_TRUE(state.finished);
    EXPECT_EQ(backend.thread_join(&thread), ESRCH);
}

UTEST(MongocThread, InvalidArguments) {
    const auto& backend = cdriver::GetThreadBackend();
    void* thread{};
    EXPECT_EQ(backend.thread_create(nullptr, WaitForFinish, nullptr), EINVAL);
    EXPECT_EQ(backend.thread_create(&thread, nullptr, nullptr), EINVAL);
    EXPECT_EQ(backend.thread_join(&thread), ESRCH);
}

UTEST(MongocThread, JoinDoesNotBlockThreadCreation) {
    const auto& backend = cdriver::GetThreadBackend();
    engine::SingleConsumerEvent create_child;
    void* parent{};
    ASSERT_EQ(
        backend.thread_create(
            &parent,
            [](void* arg) -> void* {
                EXPECT_TRUE(static_cast<engine::SingleConsumerEvent*>(arg)->WaitForEvent());
                const auto& backend = cdriver::GetThreadBackend();
                void* child{};
                EXPECT_EQ(backend.thread_create(&child, [](void*) -> void* { return nullptr; }, nullptr), 0);
                EXPECT_EQ(backend.thread_join(&child), 0);
                return nullptr;
            },
            &create_child
        ),
        0
    );
    auto joiner = utils::Async("join-parent", [&] {
        create_child.Send();
        return backend.thread_join(&parent);
    });
    EXPECT_EQ(joiner.Get(), 0);
}

UTEST(MongocThread, JoinIgnoresCallerCancellation) {
    const auto& backend = cdriver::GetThreadBackend();
    ThreadState state;
    void* thread{};
    ASSERT_EQ(backend.thread_create(&thread, WaitForFinish, &state), 0);
    EXPECT_TRUE(state.started.WaitForEvent());
    engine::SingleConsumerEvent joining;
    auto joiner = utils::Async("cancelled-join", [&] {
        engine::current_task::RequestCancel();
        joining.Send();
        return backend.thread_join(&thread);
    });
    EXPECT_TRUE(joining.WaitForEvent());
    state.finish.Send();
    EXPECT_EQ(joiner.Get(), 0);
    EXPECT_TRUE(state.finished);
}

UTEST(MongocThread, BackgroundDoesNotInheritRequestDeadline) {
    const auto& backend = cdriver::GetThreadBackend();
    server::request::kTaskInheritedData.Set({{}, "mongo-thread-test", {}, engine::Deadline::Passed()});
    void* thread{};
    ASSERT_EQ(
        backend.thread_create(
            &thread,
            [](void*) -> void* {
                EXPECT_FALSE(server::request::GetTaskInheritedDeadline().IsReachable());
                EXPECT_TRUE(engine::current_task::impl::IsCritical());
                return nullptr;
            },
            nullptr
        ),
        0
    );
    EXPECT_EQ(backend.thread_join(&thread), 0);
}

UTEST(MongocThread, SelfJoin) {
    struct State {
        engine::SingleConsumerEvent ready;
        void* thread{};
    } state;
    const auto& backend = cdriver::GetThreadBackend();
    ASSERT_EQ(
        backend.thread_create(
            &state.thread,
            [](void* arg) -> void* {
                auto& state = *static_cast<State*>(arg);
                EXPECT_TRUE(state.ready.WaitForEvent());
                EXPECT_EQ(cdriver::GetThreadBackend().thread_join(&state.thread), EDEADLK);
                return nullptr;
            },
            &state
        ),
        0
    );
    state.ready.Send();
    EXPECT_EQ(backend.thread_join(&state.thread), 0);
}

UTEST(MongocCondition, TimeoutWhileCancelled) {
    BackendCondition condition;
    auto waiter = utils::Async("cancelled-condition", [&] {
        engine::current_task::RequestCancel();
        condition.backend.mutex_lock(&condition.mutex);
        const auto result = condition.backend.cond_timedwait(&condition.cond, &condition.mutex, 1);
        condition.backend.mutex_unlock(&condition.mutex);
        return result;
    });
    EXPECT_EQ(waiter.Get(), ETIMEDOUT);
    EXPECT_TRUE(condition.backend.cond_is_timedout(ETIMEDOUT));
    EXPECT_FALSE(condition.backend.cond_is_timedout(0));
}

UTEST(MongocCondition, NotificationWhileCancelled) {
    BackendCondition condition;
    engine::SingleConsumerEvent waiting;
    bool ready = false;
    auto waiter = utils::Async("notified-condition", [&] {
        engine::current_task::RequestCancel();
        condition.backend.mutex_lock(&condition.mutex);
        waiting.Send();
        while (!ready) {
            EXPECT_EQ(condition.backend.cond_wait(&condition.cond, &condition.mutex), 0);
        }
        condition.backend.mutex_unlock(&condition.mutex);
    });
    EXPECT_TRUE(waiting.WaitForEvent());
    condition.backend.mutex_lock(&condition.mutex);
    ready = true;
    condition.backend.cond_signal(&condition.cond);
    condition.backend.mutex_unlock(&condition.mutex);
    waiter.Get();
}

namespace {

void* MakeProcessLifetimeOnceStorage() {
    // The backend retains storage addresses; repeated tests must never reuse them.
    static std::deque<void*> slots;
    return &slots.emplace_back(nullptr);
}

struct OnceTestState {
    engine::SingleConsumerEvent entered;
    engine::SingleConsumerEvent finish;
    std::atomic<int> calls{0};
    int value{0};
};

OnceTestState* once_test_state{};

void InitializeOnceWithWait() {
    auto& state = *once_test_state;
    ++state.calls;
    state.entered.Send();
    EXPECT_TRUE(state.finish.WaitForEvent());
    state.value = 42;
}

}  // namespace

UTEST_MT(MongocOnce, ConcurrentWaitersAndCancellation, 4) {
    const auto& backend = cdriver::GetThreadBackend();
    void* const once = MakeProcessLifetimeOnceStorage();
    OnceTestState state;
    once_test_state = &state;
    auto initializer = utils::Async("once-init", [&] { backend.once(once, InitializeOnceWithWait); });
    EXPECT_TRUE(state.entered.WaitForEvent());
    std::vector<engine::TaskWithResult<void>> waiters;
    waiters.reserve(16);
    for (int i = 0; i < 16; ++i) {
        waiters.push_back(utils::Async("once-waiter", [&] {
            engine::current_task::RequestCancel();
            backend.once(once, InitializeOnceWithWait);
            EXPECT_EQ(state.value, 42);
        }));
    }
    state.finish.Send();
    initializer.Get();
    for (auto& waiter : waiters) {
        waiter.Get();
    }
    EXPECT_EQ(state.calls, 1);
    once_test_state = nullptr;
}

TEST(MongocOnce, NativeInitializationThenCoroutine) {
    const auto& backend = cdriver::GetThreadBackend();
    void* const once = MakeProcessLifetimeOnceStorage();
    backend.once(once, [] {});
    engine::RunStandalone([&] { backend.once(once, [] { FAIL() << "Initializer ran twice"; }); });
}

UTEST(MongocSharedMutex, ReadersAndWriterYield) {
    const auto& backend = cdriver::GetThreadBackend();
    int mutex{};
    backend.shared_mutex_init(&mutex);
    backend.shared_mutex_lock_shared(&mutex);
    engine::SingleConsumerEvent reader_locked;
    engine::SingleConsumerEvent release_reader;
    auto reader = utils::Async("shared-reader", [&] {
        backend.shared_mutex_lock_shared(&mutex);
        reader_locked.Send();
        EXPECT_TRUE(release_reader.WaitForEvent());
        backend.shared_mutex_unlock_shared(&mutex);
    });
    EXPECT_TRUE(reader_locked.WaitForEvent());
    engine::SingleConsumerEvent writer_started;
    auto writer = utils::Async("exclusive-writer", [&] {
        writer_started.Send();
        backend.shared_mutex_lock(&mutex);
        backend.shared_mutex_unlock(&mutex);
    });
    EXPECT_TRUE(writer_started.WaitForEvent());
    EXPECT_FALSE(writer.IsFinished());
    backend.shared_mutex_unlock_shared(&mutex);
    release_reader.Send();
    reader.Get();
    writer.Get();
    backend.shared_mutex_destroy(&mutex);
}

TEST(MongocSharedMutex, GlobalLifecycle) {
    const cdriver::MongocGlobalLifecycleScope scope;
    const auto& backend = cdriver::GetThreadBackend();
    int mutex{};
    backend.shared_mutex_init(&mutex);
    backend.shared_mutex_lock(&mutex);
    backend.shared_mutex_unlock(&mutex);
    backend.shared_mutex_lock_shared(&mutex);
    backend.shared_mutex_unlock_shared(&mutex);
    backend.shared_mutex_destroy(&mutex);
}

UTEST(MongocSleep, YieldsToOtherTasks) {
    const auto& backend = cdriver::GetThreadBackend();
    bool another_task_ran = false;
    auto other = utils::Async("during-mongo-sleep", [&] { another_task_ran = true; });
    backend.sleep(1000);
    EXPECT_TRUE(another_task_ran);
    other.Get();
}

USERVER_NAMESPACE_END
