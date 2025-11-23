#include <userver/utest/utest.hpp>

#include <engine/deadlock_detector.hpp>
#include <userver/engine/run_standalone.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/mutex.hpp>

USERVER_NAMESPACE_BEGIN

TEST(DeadlockDetectorDeathTest, Smoke) {
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
    CycleDetected(std::vector<const engine::deadlock_detector::Actor*> cycle)
        : std::runtime_error(""),
          cycle(std::move(cycle))
    {}

    std::vector<const engine::deadlock_detector::Actor*> cycle;
};

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXPECT_THROW_CYCLE(cmd, ...)                                                             \
    try {                                                                                        \
        cmd;                                                                                     \
        FAIL() << "Deadlock was expected, but not found";                                        \
    } catch (const CycleDetected& c) {                                                           \
        EXPECT_EQ(c.cycle, (std::vector<const engine::deadlock_detector::Actor*>{__VA_ARGS__})); \
    }

class MockState final : public engine::deadlock_detector::StateBase {
public:
    MockState()
        : StateBase(engine::DeadlockDetector::kOn)
    {}

protected:
    void OnCycleFound(const std::vector<const engine::deadlock_detector::Actor*>& cycle) override {
        throw CycleDetected(cycle);
    }
};

class MockActor final : public engine::deadlock_detector::Actor {
    utils::StringLiteral GetActorType() const override { return "mock"; }
};

UTEST(DeadlockDetector, Ctr) {
    MockState state;
    SUCCEED();
}

UTEST(DeadlockDetector, NoCycle) {
    MockState state;
    MockActor a1;
    MockActor a2;

    state.HookBeforeAddDependency(a1, a2);
    state.HookBeforeRemoveDependency(a1, a2);

    SUCCEED();
}

UTEST(DeadlockDetector, Cycle2) {
    MockState state;
    MockActor a1;
    MockActor a2;

    state.HookBeforeAddDependency(a1, a2);
    EXPECT_THROW_CYCLE(state.HookBeforeAddDependency(a2, a1), &a1, &a2);
}

UTEST(DeadlockDetector, Cycle3) {
    MockState state;
    MockActor a1;
    MockActor a2;
    MockActor a3;

    state.HookBeforeAddDependency(a1, a2);
    state.HookBeforeAddDependency(a2, a3);
    EXPECT_THROW_CYCLE(state.HookBeforeAddDependency(a3, a1), &a1, &a3, &a2);
}

UTEST(DeadlockDetector, RightCycle) {
    MockState state;
    MockActor a1;
    MockActor a2;
    MockActor b1;
    MockActor b2;

    state.HookBeforeAddDependency(a1, a2);
    state.HookBeforeAddDependency(b1, b2);
    EXPECT_THROW_CYCLE(state.HookBeforeAddDependency(a2, a1), &a1, &a2);
}

USERVER_NAMESPACE_END
