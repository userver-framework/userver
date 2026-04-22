#pragma once

#include <atomic>

#include <userver/engine/deadline.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>

#include <engine/deadlock_detector.hpp>
#include <engine/impl/wait_list.hpp>
#include <engine/impl/wait_list_light.hpp>
#include <engine/task/task_context.hpp>
#include <userver/compiler/impl/tsan.hpp>
#include <userver/engine/impl/actor.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

template <class Awaiters>
class MutexImpl : public deadlock_detector::Actor {
public:
    MutexImpl();
    ~MutexImpl();

    MutexImpl(const MutexImpl&) = delete;
    MutexImpl(MutexImpl&&) = delete;
    MutexImpl& operator=(const MutexImpl&) = delete;
    MutexImpl& operator=(MutexImpl&&) = delete;

    void lock();

    void unlock();

    bool try_lock();

    bool try_lock_until(Deadline deadline);

    utils::StringLiteral GetActorType() const override { return "Mutex"; }

private:
    class MutexWaitStrategy;

    bool LockFastPath(TaskContext&) noexcept;
    bool LockSlowPath(TaskContext&, Deadline);

    std::atomic<TaskContext*> owner_;
    Awaiters lock_awaiters_;
};

template <>
class MutexImpl<WaitList>::MutexWaitStrategy final : public WaitStrategy {
public:
    MutexWaitStrategy(MutexImpl<WaitList>& mutex, TaskContext& current)
        : mutex_(mutex),
          current_(current),
          awaiter_token_(mutex_.lock_awaiters_)
    {}

    EarlyNotify SetupWakeups() override {
        WaitList::Lock lock(mutex_.lock_awaiters_);
        if (mutex_.LockFastPath(current_)) {
            return EarlyNotify{true};
        }
        // A race is not possible here, because check + Append is performed under
        // WaitList::Lock, and notification also takes WaitList::Lock.
        mutex_.lock_awaiters_.Append(lock, &current_, current_.GetAwaiterContext());
        return EarlyNotify{false};
    }

    void DisableWakeups() noexcept override {
        WaitList::Lock lock(mutex_.lock_awaiters_);
        mutex_.lock_awaiters_.Remove(lock, current_, current_.GetAwaiterContext());
    }

private:
    MutexImpl<WaitList>& mutex_;
    TaskContext& current_;
    const WaitList::AwaitersScopeCounter awaiter_token_;
};

template <>
class MutexImpl<WaitListLight>::MutexWaitStrategy final : public WaitStrategy {
public:
    MutexWaitStrategy(MutexImpl<WaitListLight>& mutex, TaskContext& current)
        : mutex_(mutex),
          current_(current)
    {}

    EarlyNotify SetupWakeups() override {
        if (TryLock()) {
            return EarlyNotify{true};
        }
        mutex_.lock_awaiters_.Append(&current_, current_.GetAwaiterContext());
        if (mutex_.owner_.load() == nullptr) {
            mutex_.lock_awaiters_.Remove(current_, current_.GetAwaiterContext());
            return EarlyNotify{true};
        }
        return EarlyNotify{false};
    }

    void DisableWakeups() noexcept override { mutex_.lock_awaiters_.Remove(current_, current_.GetAwaiterContext()); }

private:
    bool TryLock() {
        TaskContext* expected = nullptr;
        if (mutex_.owner_.compare_exchange_strong(expected, &current_, std::memory_order_relaxed)) {
            return true;
        }
        UINVARIANT(expected != &current_, "MutexImpl is locked twice from the same task");
        return false;
    }

    MutexImpl<WaitListLight>& mutex_;
    TaskContext& current_;
};

template <class Awaiters>
MutexImpl<Awaiters>::MutexImpl()
    : owner_(nullptr)
{
#if USERVER_IMPL_HAS_TSAN
    __tsan_mutex_create(this, __tsan_mutex_not_static);
#endif
}

template <class Awaiters>
MutexImpl<Awaiters>::~MutexImpl() {
    UASSERT(!owner_);
#if USERVER_IMPL_HAS_TSAN
    __tsan_mutex_destroy(this, __tsan_mutex_not_static);
#endif
}

template <class Awaiters>
bool MutexImpl<Awaiters>::LockFastPath(TaskContext& current) noexcept {
    TaskContext* expected = nullptr;
    return owner_.compare_exchange_strong(expected, &current, std::memory_order_acquire);
}

template <class Awaiters>
bool MutexImpl<Awaiters>::LockSlowPath(TaskContext& current, Deadline deadline) {
    const engine::TaskCancellationBlocker block_cancels;
    MutexWaitStrategy wait_manager{*this, current};
    while (true) {
        const auto wakeup_source = current.Sleep(wait_manager, deadline);
        if (owner_.load() == &current) {
            return true;
        }
        if (!HasWaitSucceeded(wakeup_source)) {
            return false;
        }
    }
    return true;
}

template <class Awaiters>
void MutexImpl<Awaiters>::lock() {
    try_lock_until(Deadline{});
}

template <class Awaiters>
void MutexImpl<Awaiters>::unlock() {
    auto& dd_state = engine::deadlock_detector::GetState();
    dd_state.OnResourceRelease(current_task::GetCurrentTaskContext(), *this);

#if USERVER_IMPL_HAS_TSAN
    __tsan_mutex_pre_unlock(this, 0);
#endif
    auto* old_owner = owner_.exchange(nullptr, std::memory_order_acq_rel);
    UASSERT(old_owner && old_owner->IsCurrent());

    if constexpr (std::is_same_v<Awaiters, WaitList>) {
        if (lock_awaiters_.GetCountOfSleepies()) {
            WaitList::Lock lock(lock_awaiters_);
            lock_awaiters_.NotifyOne(lock);
        }
    } else {
        static_assert(std::is_same_v<Awaiters, WaitListLight>);
        lock_awaiters_.NotifyOne();
    }

#if USERVER_IMPL_HAS_TSAN
    __tsan_mutex_post_unlock(this, 0);
#endif
}

template <class Awaiters>
bool MutexImpl<Awaiters>::try_lock() {
    auto& current = current_task::GetCurrentTaskContext();

#if USERVER_IMPL_HAS_TSAN
    __tsan_mutex_pre_lock(this, __tsan_mutex_try_lock);
#endif
    const auto result = LockFastPath(current);
#if USERVER_IMPL_HAS_TSAN
    __tsan_mutex_post_lock(this, __tsan_mutex_try_lock | (result ? 0 : __tsan_mutex_try_lock_failed), 0);
#endif

    auto& dd_state = engine::deadlock_detector::GetState();
    if (result) {
        dd_state.OnResourceAcquire(current, *this);
    }

    return result;
}

template <class Awaiters>
bool MutexImpl<Awaiters>::try_lock_until(Deadline deadline) {
    bool result = false;
#if USERVER_IMPL_HAS_TSAN
    __tsan_mutex_pre_lock(this, __tsan_mutex_try_lock);

    // Use ScopeGuard to be sure __tsan_mutex_post_lock() is called
    // even in case of exception in UINVARIANT() below
    const utils::FastScopeGuard stop_wait([this, &result]() noexcept {
        __tsan_mutex_post_lock(this, __tsan_mutex_try_lock | (result ? 0 : __tsan_mutex_try_lock_failed), 0);
    });
#endif

    std::optional<engine::deadlock_detector::WaitScope> scope;
    if (!deadline.IsReachable()) {
        scope.emplace(*this);
    }

    auto& current = current_task::GetCurrentTaskContext();
    UINVARIANT(
        owner_.load() != &current,
        "engine::mutex self deadlock detected! Current coroutine tried to lock a mutex while holding the same mutex."
    );

    result = LockFastPath(current) || LockSlowPath(current, deadline);

    scope.reset();

    auto& dd_state = engine::deadlock_detector::GetState();
    if (result) {
        dd_state.OnResourceAcquire(current, *this);
    }

    return result;
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
