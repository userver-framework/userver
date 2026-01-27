#include <userver/utest/utest.hpp>

#include <engine/deadlock_detector.hpp>
#include <userver/engine/run_standalone.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/mutex.hpp>

USERVER_NAMESPACE_BEGIN

TEST(DeadlockDetectorDeathTest, Smoke) {
    testing::FLAGS_gtest_death_test_style = "threadsafe";

    engine::TaskProcessorPoolsConfig config;
    config.deadlock_detector = engine::DeadlockDetector::kOn;

    engine::RunStandalone(1, config, [] {
        engine::Mutex mutex;
        std::unique_lock lock(mutex);

        auto task = engine::AsyncNoSpan([&mutex] { std::unique_lock lock(mutex); });
        EXPECT_DEATH(
            task.Get(),
            R"(Mutex \(ptr=0x[a-z0-9]*\) => Task \(ptr=0x[a-z0-9]*\) => Task \(ptr=0x[a-z0-9]*\))"
        );
    });
}

struct CycleDetected : public std::runtime_error {
    CycleDetected(std::vector<const engine::impl::deadlock_detector::Actor*> cycle)
        : std::runtime_error(""),
          cycle(std::move(cycle))
    {}

    std::vector<const engine::impl::deadlock_detector::Actor*> cycle;
};

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXPECT_THROW_CYCLE(cmd, ...)                                                                   \
    try {                                                                                              \
        cmd;                                                                                           \
        FAIL() << "Deadlock was expected, but not found";                                              \
    } catch (const CycleDetected& c) {                                                                 \
        EXPECT_EQ(c.cycle, (std::vector<const engine::impl::deadlock_detector::Actor*>{__VA_ARGS__})); \
    }

class MockState final : public engine::deadlock_detector::StateBase {
public:
    MockState()
        : StateBase(engine::DeadlockDetector::kOn)
    {}

protected:
    void OnCycleFound(const std::vector<const engine::impl::deadlock_detector::Actor*>& cycle) override {
        throw CycleDetected(cycle);
    }
};

class MockActor final : public engine::impl::deadlock_detector::Actor {
    utils::StringLiteral GetActorType() const override { return "mock"; }
};

UTEST(DeadlockDetector, Ctr) {
    MockState state;
    SUCCEED();
}

UTEST(DeadlockDetector, NoCycle) {
    MockState state;
    MockActor actor;
    MockActor resource;

    state.OnResourceAcquire(actor, resource);
    state.OnResourceRelease(actor, resource);

    state.OnWaitForResourceStart(actor, resource);
    state.OnWaitForResourceFinish(actor, resource);

    SUCCEED();
}

UTEST(DeadlockDetector, Cycle2AcquireWait) {
    MockState state;
    MockActor actor;
    MockActor resource;

    // actor -> resource -> actor
    state.OnResourceAcquire(actor, resource);
    EXPECT_THROW_CYCLE(state.OnWaitForResourceStart(actor, resource), &resource, &actor);
}

UTEST(DeadlockDetector, Cycle2WaitAcquire) {
    MockState state;
    MockActor actor;
    MockActor resource;

    // resource -> actor -> resource
    state.OnWaitForResourceStart(actor, resource);
    EXPECT_THROW_CYCLE(state.OnResourceAcquire(actor, resource), &actor, &resource);
}

UTEST(DeadlockDetector, Cycle3) {
    MockState state;
    MockActor actor1;
    MockActor actor2;
    MockActor resource;

    // resource -> actor1 -> actor2 -> resource
    state.OnResourceAcquire(actor1, resource);
    state.OnWaitForResourceStart(actor2, resource);
    EXPECT_THROW_CYCLE(state.OnWaitForResourceStart(actor1, actor2), &actor2, &actor1, &resource);
}

UTEST(DeadlockDetector, RightCycle) {
    MockState state;
    MockActor actor1;
    MockActor actor2;
    MockActor resource1;
    MockActor resource2;

    // actor1 -> resource1 -> actor1
    // resource2 -> actor2
    state.OnResourceAcquire(actor1, resource1);
    state.OnResourceAcquire(actor2, resource2);
    EXPECT_THROW_CYCLE(state.OnWaitForResourceStart(actor1, resource1), &resource1, &actor1);
}

UTEST(DeadlockDetector, ClassicMutualDeadlock) {
    MockState state;
    MockActor actor1;
    MockActor actor2;
    MockActor resource1;
    MockActor resource2;

    // actor1 -> resource2 -> actor2 -> resource1 -> actor1
    state.OnResourceAcquire(actor1, resource1);
    state.OnResourceAcquire(actor2, resource2);
    state.OnWaitForResourceStart(actor1, resource2);
    EXPECT_THROW_CYCLE(state.OnWaitForResourceStart(actor2, resource1), &resource1, &actor2, &resource2, &actor1);
}

UTEST(DeadlockDetector, NoCycleOnDiamond) {
    /* Tests processing of a diamond shape:
    //        start
    //          |
    //          0
    //         / \
    //        /   \
    //       1     4
    //        \   /
    //         \ /
    //          2
    //          |
    //          3
    */
    MockState state;
    MockActor start;
    MockActor v0;
    MockActor v1;
    MockActor v2;
    MockActor v3;
    MockActor v4;

    state.OnWaitForResourceStart(v0, v1);
    state.OnWaitForResourceStart(v0, v4);
    state.OnWaitForResourceStart(v1, v2);
    state.OnWaitForResourceStart(v4, v2);
    state.OnWaitForResourceStart(v2, v3);
    state.OnWaitForResourceStart(start, v0);

    SUCCEED();
}

UTEST(DeadlockDetector, ReentrantNoCycle) {
    MockState state;
    MockActor actor;
    MockActor resource;

    state.OnReentrantResourceAcquire(actor, resource);
    state.OnReentrantResourceAcquire(actor, resource);
    state.OnResourceRelease(actor, resource);
    state.OnResourceRelease(actor, resource);

    state.OnWaitForResourceStart(actor, resource);
    state.OnWaitForResourceFinish(actor, resource);

    SUCCEED();
}

UTEST(DeadlockDetector, Cycle2ReentrantAcquireWait) {
    MockState state;
    MockActor actor;
    MockActor resource;

    // actor -> resource -> actor
    state.OnReentrantResourceAcquire(actor, resource);
    state.OnReentrantResourceAcquire(actor, resource);
    EXPECT_THROW_CYCLE(state.OnWaitForResourceStart(actor, resource), &resource, &actor);
}

USERVER_NAMESPACE_END
