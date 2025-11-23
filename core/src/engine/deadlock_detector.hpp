#pragma once

#include <vector>

#include <engine/task/task_context.hpp>

#include <engine/coro/pool_config.hpp>
#include <engine/deadlock_detector/actor.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::deadlock_detector {

class StateBase {
public:
    explicit StateBase(DeadlockDetector dd);

    virtual ~StateBase();

    void HookBeforeAddDependency(const Actor& subject, const Actor& object);

    void HookBeforeRemoveDependency(const Actor& subject, const Actor& object) noexcept;

    void HookActorDestroy(const Actor& object);

protected:
    virtual void OnCycleFound(const std::vector<const Actor*>& cycle) = 0;

private:
    struct Impl;
    utils::FastPimpl<Impl, 232, 8> impl_;
};

class State final : public StateBase {
protected:
    using StateBase::StateBase;

    void OnCycleFound(const std::vector<const Actor*>& cycle) override;
};

// The state is global to the process
// because a deadlock may spread across multiple TaskProcessors
State& GetState();

class WaitScope final {
public:
    explicit WaitScope(const Actor& a);
    WaitScope(WaitScope&&) = delete;
    ~WaitScope();

private:
    const Actor& actor_;
};

}  // namespace engine::deadlock_detector

USERVER_NAMESPACE_END
