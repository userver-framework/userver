#pragma once

#include <vector>

#include <engine/task/task_context.hpp>

#include <engine/coro/pool_config.hpp>
#include <userver/engine/impl/actor.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::deadlock_detector {

class StateBase {
public:
    using Actor = engine::impl::deadlock_detector::Actor;

    explicit StateBase(DeadlockDetector dd);

    virtual ~StateBase();

    // Actor acquires the resource.
    // The actor could acquire the same resource only once.
    void OnResourceAcquire(const Actor& owner, const Actor& resource);

    // Current task acquires the resource.
    // The current task could acquire the same resource only once.
    void OnResourceAcquire(const Actor& resource);

    // Actor acquires the resource.
    // The actor could acquire the same resource multiple times.
    void OnReentrantResourceAcquire(const Actor& owner, const Actor& resource);

    // Current task acquires the resource.
    // The current task could acquire the same resource multiple times.
    void OnReentrantResourceAcquire(const Actor& resource);

    // Actor releases the resource.
    void OnResourceRelease(const Actor& owner, const Actor& resource) noexcept;

    // Current task releases the resource.
    void OnResourceRelease(const Actor& resource) noexcept;

    // Actor waits for the resource.
    void OnWaitForResourceStart(const Actor& waiting, const Actor& resource);

    // Current task waits for the resource.
    void OnWaitForResourceStart(const Actor& resource);

    // Actor finishes waiting for the resource.
    void OnWaitForResourceFinish(const Actor& waiting, const Actor& resource) noexcept;

    // Current task finishes waiting for the resource.
    void OnWaitForResourceFinish(const Actor& resource) noexcept;

    void OnActorDestroy(const Actor& actor);

protected:
    virtual void OnCycleFound(const std::vector<const Actor*>& cycle) = 0;

private:
    void AddDependency(const Actor& from, const Actor& to, bool allow_repeated_deps);

    void RemoveDependency(const Actor& from, const Actor& to) noexcept;

    struct Impl;
    utils::FastPimpl<Impl, 232, 8> impl_;
};

class State final : public StateBase {
protected:
    using StateBase::StateBase;

    void OnCycleFound(const std::vector<const StateBase::Actor*>& cycle) override;
};

// The state is global to the process
// because a deadlock may spread across multiple TaskProcessors
State& GetState();

class WaitScope final {
public:
    explicit WaitScope(const engine::impl::deadlock_detector::Actor& resource);
    WaitScope(WaitScope&&) = delete;
    ~WaitScope();

private:
    const engine::impl::deadlock_detector::Actor& resource_;
};

}  // namespace engine::deadlock_detector

USERVER_NAMESPACE_END
