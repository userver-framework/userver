#include <userver/utest/utest.hpp>

#include <functional>
#include <optional>
#include <utility>

#include <userver/engine/async.hpp>
#include <userver/engine/single_use_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/engine/task/inherited_variable.hpp>
#include <userver/engine/task/local_variable.hpp>
#include <userver/utils/async.hpp>

#include <engine/task/task_processor.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class LogStringGuard final {
public:
    LogStringGuard(std::string& destination, std::string source)
        : destination_(destination),
          source_(std::move(source))
    {}

    ~LogStringGuard() { destination_ += source_; }

private:
    std::string& destination_;
    std::string source_;
};

engine::TaskLocalVariable<int> kIntVariable;
engine::TaskLocalVariable<std::optional<LogStringGuard>> kGuardX;
engine::TaskLocalVariable<std::optional<LogStringGuard>> kGuardY;
engine::TaskLocalVariable<std::optional<LogStringGuard>> kGuardZ;

}  // namespace

UTEST(TaskLocalVariable, SetGet) {
    EXPECT_FALSE(kIntVariable.GetOptional());

    *kIntVariable = 1;
    EXPECT_EQ(1, *kIntVariable);
    EXPECT_TRUE(kIntVariable.GetOptional());

    engine::Yield();
    EXPECT_EQ(1, *kIntVariable);

    *kIntVariable = 2;
    EXPECT_EQ(2, *kIntVariable);

    engine::Yield();
    EXPECT_EQ(2, *kIntVariable);
}

UTEST(TaskLocalVariable, TwoTask) {
    *kIntVariable = 1;

    auto task = engine::AsyncNoTracing([] {
        *kIntVariable = 2;
        EXPECT_EQ(2, *kIntVariable);

        engine::Yield();
        EXPECT_EQ(2, *kIntVariable);

        *kIntVariable = 3;
        EXPECT_EQ(3, *kIntVariable);

        engine::Yield();
        EXPECT_EQ(3, *kIntVariable);
    });

    engine::Yield();
    EXPECT_EQ(1, *kIntVariable);

    *kIntVariable = 10;
    EXPECT_EQ(10, *kIntVariable);

    engine::Yield();
    EXPECT_EQ(10, *kIntVariable);
}

UTEST(TaskLocalVariable, MultipleThreads) {
    *kIntVariable = 1;

    auto task = engine::AsyncNoTracing([] {
        *kIntVariable = 2;
        EXPECT_EQ(2, *kIntVariable);

        engine::Yield();
        EXPECT_EQ(2, *kIntVariable);

        *kIntVariable = 3;
        EXPECT_EQ(3, *kIntVariable);

        engine::Yield();
        EXPECT_EQ(3, *kIntVariable);
    });

    engine::Yield();
    EXPECT_EQ(1, *kIntVariable);

    *kIntVariable = 10;
    EXPECT_EQ(10, *kIntVariable);

    engine::Yield();
    EXPECT_EQ(10, *kIntVariable);
}

UTEST(TaskLocalVariable, Destructor) {
    std::string destruction_order;

    utils::Async("test", [&] {
        kGuardX->emplace(destruction_order, "1");
        EXPECT_EQ(destruction_order, "");

        engine::AsyncNoTracing([&] { kGuardX->emplace(destruction_order, "2"); }).Get();

        EXPECT_EQ(destruction_order, "2");
    }).Get();

    EXPECT_EQ(destruction_order, "21");
}

UTEST(TaskLocalVariable, DestructionOrder) {
    {
        std::string destruction_order;

        engine::AsyncNoTracing([&] {
            kGuardY->emplace(destruction_order, "y");
            kGuardX->emplace(destruction_order, "x");
            kGuardZ->emplace(destruction_order, "z");
        }).Get();

        // variables are destroyed in reverse-initialization order
        EXPECT_EQ(destruction_order, "zxy");
    }

    {
        std::string destruction_order;

        engine::AsyncNoTracing([&] {
            kGuardX->emplace(destruction_order, "x");
            kGuardY->emplace(destruction_order, "y");
        }).Get();

        // different tasks may have different initialization order and utilize
        // different sets of variables
        EXPECT_EQ(destruction_order, "yx");
    }
}

namespace {

class WaitingInDestructorVariable final {
public:
    explicit WaitingInDestructorVariable(engine::SingleUseEvent& event)
        : event_(event)
    {}

    ~WaitingInDestructorVariable() { event_.WaitNonCancellable(); }

private:
    engine::SingleUseEvent& event_;
};

engine::TaskLocalVariable<std::optional<WaitingInDestructorVariable>> kWaitingInDestructorVariable;

}  // namespace

UTEST(TaskLocalVariable, WaitInDestructor) {
    engine::SingleUseEvent event;
    auto task = engine::AsyncNoTracing([&] { kWaitingInDestructorVariable->emplace(event); });

    engine::SleepFor(std::chrono::milliseconds{100});
    EXPECT_FALSE(task.IsFinished());
    EXPECT_EQ(task.GetState(), engine::TaskBase::State::kSuspended);

    event.Send();
    task.Wait();
    EXPECT_EQ(task.GetState(), engine::TaskBase::State::kCompleted);
    UEXPECT_NO_THROW(task.Get());
}

UTEST(TaskLocalVariable, WaitInDestructorCancelled) {
    engine::SingleUseEvent event;
    auto task = engine::AsyncNoTracing([&] {
        kWaitingInDestructorVariable->emplace(event);
        engine::current_task::RequestCancel();
        engine::current_task::CancellationPoint();
    });

    engine::SleepFor(std::chrono::milliseconds{100});
    EXPECT_FALSE(task.IsFinished());
    EXPECT_EQ(task.GetState(), engine::TaskBase::State::kSuspended);

    event.Send();
    task.Wait();
    EXPECT_EQ(task.GetState(), engine::TaskBase::State::kCancelled);
    UEXPECT_THROW(task.Get(), engine::TaskCancelledException);
}

namespace {

struct DtorCallback final {
    std::function<void()> on_destroy;

    DtorCallback() = default;
    DtorCallback(const DtorCallback&) = delete;
    DtorCallback& operator=(const DtorCallback&) = delete;

    ~DtorCallback() {
        if (on_destroy) {
            on_destroy();
        }
    }
};

engine::TaskLocalVariable<DtorCallback> kDtorCallback;
engine::TaskLocalVariable<int> kObservedInt;

}  // namespace

// Situation 1a: GetOptional on a live (not yet destroyed) variable.
UTEST(TaskLocalVariable, GetOptionalAlive) {
    EXPECT_EQ(kObservedInt.GetOptional(), nullptr);

    *kObservedInt = 42;
    ASSERT_NE(kObservedInt.GetOptional(), nullptr);
    EXPECT_EQ(*kObservedInt.GetOptional(), 42);
}

// Situation 1b: a variable that is initialized earlier (thus destroyed later)
// is still observable from the destructor of another task-local variable.
UTEST(TaskLocalVariable, GetOptionalAliveFromOtherVariableDestructor) {
    bool checked = false;

    engine::AsyncNoTracing([&] {
        // kObservedInt is initialized first => destroyed after kDtorCallback.
        *kObservedInt = 42;
        kDtorCallback->on_destroy = [&] {
            auto* const observed = kObservedInt.GetOptional();
            ASSERT_NE(observed, nullptr);
            EXPECT_EQ(*observed, 42);
            checked = true;
        };
    }).Get();

    EXPECT_TRUE(checked);
}

// Situation 2: GetOptional on an already-destroyed variable must return
// nullptr, not a dangling pointer.
UTEST(TaskLocalVariable, GetOptionalAfterVariableDestroyed) {
    bool checked = false;

    engine::AsyncNoTracing([&] {
        kDtorCallback->on_destroy = [&] {
            EXPECT_EQ(kObservedInt.GetOptional(), nullptr);
            checked = true;
        };
        // kObservedInt is initialized after kDtorCallback => destroyed before
        // it, so ~DtorCallback observes an already-destroyed variable.
        *kObservedInt = 42;
    }).Get();

    EXPECT_TRUE(checked);
}

// Situation 3: GetOptional on a variable whose destructor is currently
// running must return nullptr (POSIX pthread_getspecific-style: the variable
// is unset before its destructor is invoked). The destructor itself can use
// `this` if needed; other code must not observe a half-destroyed object.
UTEST(TaskLocalVariable, GetOptionalDuringOwnDestruction) {
    bool checked = false;

    engine::AsyncNoTracing([&] {
        kDtorCallback->on_destroy = [&] {
            EXPECT_EQ(kDtorCallback.GetOptional(), nullptr);
            checked = true;
        };
    }).Get();

    EXPECT_TRUE(checked);
}

namespace {

struct CallbackHolder final {
    std::function<void()> on_destroy;

    explicit CallbackHolder(std::function<void()> cb)
        : on_destroy(std::move(cb))
    {}

    CallbackHolder(const CallbackHolder&) = delete;
    CallbackHolder& operator=(const CallbackHolder&) = delete;

    ~CallbackHolder() {
        if (on_destroy) {
            on_destroy();
        }
    }
};

engine::TaskInheritedVariable<CallbackHolder> kInheritedCallback;

}  // namespace

// A task-local variable initialized from a destructor of another task-local
// variable is destroyed after that destructor completes. Same as for
// `thread_local`, [basic.stc.thread]/2: "If a variable with thread storage
// duration is initialized after a thread-local destructor has started
// executing, its destructor is scheduled to run after that destructor
// completes".
UTEST(TaskLocalVariable, DtorInitializesLocalVariable) {
    std::string order;

    engine::AsyncNoTracing([&] {
        kDtorCallback->on_destroy = [&] {
            kGuardX->emplace(order, "x");
            order += "a";
        };
    }).Get();

    // "a" (the initializing destructor completes) strictly before "x"
    EXPECT_EQ(order, "ax");
}

// A task-inherited variable initialized from a destructor of a task-local
// variable is destroyed after that destructor completes.
UTEST(TaskLocalVariable, LocalDtorInitializesInheritedVariable) {
    std::string order;

    engine::AsyncNoTracing([&] {
        kDtorCallback->on_destroy = [&] {
            kInheritedCallback.Emplace([&order] { order += "i"; });
            order += "a";
        };
    }).Get();

    EXPECT_EQ(order, "ai");
}

// A task-local variable initialized from a destructor of a task-inherited
// variable is destroyed after that destructor completes.
UTEST(TaskLocalVariable, InheritedDtorInitializesLocalVariable) {
    std::string order;

    engine::AsyncNoTracing([&] {
        kInheritedCallback.Emplace([&] {
            kGuardX->emplace(order, "x");
            order += "a";
        });
    }).Get();

    EXPECT_EQ(order, "ax");
}

// A task-inherited variable initialized from a destructor of another
// task-inherited variable is destroyed after that destructor completes.
UTEST(TaskLocalVariable, InheritedDtorInitializesInheritedVariable) {
    std::string order;

    engine::AsyncNoTracing([&] {
        kInheritedCallback.Emplace([&] {
            kInheritedCallback.Emplace([&order] { order += "i"; });
            order += "a";
        });
    }).Get();

    EXPECT_EQ(order, "ai");
}

// A child task inherits a variable on construction and keeps it alive even if:
// - the parent erases the variable before the child starts,
// - the child is cancelled before start (finishes as State::kCancelled).
// The variable is destroyed only after the child task finishes.
UTEST(TaskInheritedVariable, ErasedInParentWhileChildCancelledBeforeStart) {
    // With more than 1 worker thread the child task would start running
    // concurrently, racing with RequestCancel and Erase below.
    ASSERT_EQ(engine::current_task::GetTaskProcessor().GetWorkerCount(), 1);

    bool destroyed = false;
    kInheritedCallback.Emplace([&destroyed] { destroyed = true; });

    // The child inherits the variable on construction...
    auto task = utils::Async("child", [] { FAIL() << "the task must not start"; });
    // ...and is cancelled before it gets a chance to start.
    task.RequestCancel();

    // Hide the variable from the parent. The child still holds a reference.
    kInheritedCallback.Erase();
    EXPECT_FALSE(destroyed);

    task.Wait();
    EXPECT_EQ(task.GetState(), engine::TaskBase::State::kCancelled);
    // The child dropped the last reference when it finished.
    EXPECT_TRUE(destroyed);
}

USERVER_NAMESPACE_END
