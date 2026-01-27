#include <userver/engine/shared_mutex.hpp>

#include <engine/deadlock_detector.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace {
constexpr auto kWriterLock = std::numeric_limits<Semaphore::Counter>::max();
}

SharedMutex::SharedMutex()
    : semaphore_(kWriterLock),
      registered_writers_count_(0)
{}

SharedMutex::~SharedMutex() { UASSERT(registered_writers_count_ == 0); }

void SharedMutex::lock() {
    const auto ok = try_lock_until(Deadline{});
    UASSERT(ok);
}

void SharedMutex::unlock() {
    auto& dd_state = deadlock_detector::GetState();
    dd_state.OnResourceRelease(mutex_actor_);

    const utils::ScopeGuard stop_wait([this] { UnregisterWriter(DeadlockDetectorTrackingStatus::kTracked); });

    semaphore_.unlock_shared_count(kWriterLock);
}

void SharedMutex::UnregisterWriter(DeadlockDetectorTrackingStatus tracking_status) {
    /*
     * If we're the last writer, notify readers.
     * If we're not the last, do nothing: readers are still waiting,
     * the next writer will eventually lock the semaphore.
     */
    if (tracking_status == DeadlockDetectorTrackingStatus::kTracked) {
        auto& dd_state = deadlock_detector::GetState();
        dd_state.OnResourceRelease(registered_writers_actor_);
    }
    auto writers_left = registered_writers_count_.fetch_sub(1, std::memory_order_relaxed);
    UASSERT_MSG(writers_left > 0, "unlock without lock");
    if (writers_left == 1) {
        const engine::TaskCancellationBlocker blocker;
        {
            const std::lock_guard<Mutex> lock(registered_writers_count_mutex_);
        }
        registered_writers_count_cv_.NotifyAll();
    }
}

bool SharedMutex::try_lock_until(Deadline deadline) {
    registered_writers_count_.fetch_add(1, std::memory_order_relaxed);

    DeadlockDetectorTrackingStatus tracking_status = DeadlockDetectorTrackingStatus::kUntracked;
    auto& dd_state = deadlock_detector::GetState();
    if (!deadline.IsReachable()) {
        dd_state.OnResourceAcquire(registered_writers_actor_);
        tracking_status = DeadlockDetectorTrackingStatus::kTracked;
    }

    utils::ScopeGuard stop_wait([this, tracking_status] { UnregisterWriter(tracking_status); });

    // We put the following code block into a nested scope because wait_scope has to be destroyed before the
    // OnResourceAcquire call. Otherwise we will get a false self-deadlock.
    {
        std::optional<deadlock_detector::WaitScope> wait_scope;
        if (!deadline.IsReachable()) {
            wait_scope.emplace(mutex_actor_);
        }
        const bool locked = semaphore_.try_lock_shared_until_count(deadline, kWriterLock);
        if (!locked) {
            return false;
        }
    }
    stop_wait.Release();

    dd_state.OnResourceAcquire(mutex_actor_);
    if (tracking_status == DeadlockDetectorTrackingStatus::kUntracked) {
        dd_state.OnResourceAcquire(registered_writers_actor_);
    }
    return true;
}

bool SharedMutex::try_lock() { return try_lock_until(Deadline::Passed()); }

void SharedMutex::lock_shared() {
    WaitForNoRegisteredWriters(Deadline{});
    /*
     * There is a deliberate TOCTOU race between "wait for no writers" and
     * "ok, now let's lock" - it's just a cheap way to avoid writers starvation.
     * If one or two readers sneak just before a writer out of turn,
     * we just don't care.
     *
     * Also we do not track possible blocking wait on semaphore in deadlock detector.
     * An attempt to track it there may lead to a false deadlock.
     */
    semaphore_.lock_shared();

    auto& dd_state = deadlock_detector::GetState();
    dd_state.OnReentrantResourceAcquire(mutex_actor_);
}

void SharedMutex::unlock_shared() {
    auto& dd_state = deadlock_detector::GetState();
    dd_state.OnResourceRelease(mutex_actor_);
    semaphore_.unlock_shared();
}

void SharedMutex::unlock_and_lock_shared() {
    const utils::ScopeGuard stop_wait([this] { UnregisterWriter(DeadlockDetectorTrackingStatus::kTracked); });

    semaphore_.unlock_shared_count(kWriterLock - 1);
}

bool SharedMutex::try_lock_shared() {
    if (HasRegisteredWriter()) {
        return false;
    }
    const bool locked = semaphore_.try_lock_shared();
    if (!locked) {
        return false;
    }
    auto& dd_state = deadlock_detector::GetState();
    dd_state.OnReentrantResourceAcquire(mutex_actor_);
    return true;
}

bool SharedMutex::try_lock_shared_until(Deadline deadline) {
    if (!WaitForNoRegisteredWriters(deadline)) {
        return false;
    }

    /* Same deliberate race, see comment in lock_shared() */
    if (!semaphore_.try_lock_shared_until(deadline)) {
        return false;
    }
    auto& dd_state = deadlock_detector::GetState();
    dd_state.OnReentrantResourceAcquire(mutex_actor_);
    return true;
}

bool SharedMutex::HasRegisteredWriter() const noexcept { return registered_writers_count_.load() > 0; }

bool SharedMutex::WaitForNoRegisteredWriters(Deadline deadline) {
    /* Fast path */
    if (registered_writers_count_ == 0) {
        return true;
    }

    std::optional<deadlock_detector::WaitScope> scope;
    if (!deadline.IsReachable()) {
        scope.emplace(registered_writers_actor_);
    }
    std::unique_lock<Mutex> lock(registered_writers_count_mutex_);
    return registered_writers_count_cv_.WaitUntil(lock, deadline, [this] { return registered_writers_count_ == 0; });
}

utils::StringLiteral SharedMutex::RegisteredWritersActor::GetActorType() const {
    return "SharedMutex (registered writers)";
}

utils::StringLiteral SharedMutex::MutexActor::GetActorType() const { return "SharedMutex"; }

}  // namespace engine

USERVER_NAMESPACE_END
