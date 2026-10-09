#include <storages/mongo/cdriver_experimental/thread.hpp>

#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <utility>

#include <fmt/format.h>

#include <userver/compiler/thread_local.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/multi_consumer_event.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/shared_mutex.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/engine/task/local_variable.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/async.hpp>

namespace {

namespace engine = USERVER_NAMESPACE::engine;
namespace utils = USERVER_NAMESPACE::utils;

using ThreadId = unsigned long int;

USERVER_NAMESPACE::compiler::ThreadLocal global_lifecycle_scope = [] { return false; };
engine::TaskLocalVariable<ThreadId> current_thread_id;

struct OnceState {
    std::atomic<bool> started{false};
    std::atomic<bool> finished{false};
    engine::MultiConsumerEvent event;
    OnceState* next{nullptr};
};

class OnceStates final {
public:
    ~OnceStates() {
        auto* state = head_.load();
        while (state) {
            UINVARIANT(state->finished.load(), "MongoDB initialization must finish before shutdown");
            auto* next = state->next;
            delete state;
            state = next;
        }
    }

    void Add(OnceState& state) {
        state.next = head_.load();
        while (!head_.compare_exchange_weak(state.next, &state)) {
        }
    }

private:
    std::atomic<OnceState*> head_{nullptr};
};

OnceState& GetOnceState(void** once) {
    static OnceStates states;
    std::atomic_ref<void*> pointer{*once};
    void* state = pointer.load(std::memory_order_acquire);
    if (!state) {
        auto candidate = std::make_unique<OnceState>();
        if (pointer.compare_exchange_strong(state, candidate.get(), std::memory_order_acq_rel)) {
            state = candidate.release();
            states.Add(*static_cast<OnceState*>(state));
        }
    }
    return *static_cast<OnceState*>(state);
}

struct ThreadRegistry {
    ~ThreadRegistry() { UINVARIANT(tasks.empty(), "MongoDB tasks must be joined before engine shutdown"); }

    engine::Mutex mutex;
    ThreadId next_id{1};
    std::map<ThreadId, engine::TaskWithResult<void>> tasks;
};

ThreadRegistry& GetThreadRegistry() {
    static ThreadRegistry registry;
    return registry;
}

bool IsGlobalLifecycleScope() {
    if (engine::current_task::IsTaskProcessorThread()) {
        return false;
    }
    auto scope = global_lifecycle_scope.Use();
    UINVARIANT(*scope, "MongoDB mutex used outside a coroutine or global init/cleanup");
    return true;
}

struct BackendMutex {
    engine::Mutex mutex;
    std::atomic<bool> locked{false};

    void MarkLocked() {
        UINVARIANT(!locked.exchange(true), "Concurrent MongoDB global lifecycle access or recursive mutex lock");
    }

    void MarkUnlocked() { UINVARIANT(locked.exchange(false), "Unlocking an unlocked MongoDB mutex"); }
};

struct BackendConditionVariable {
    engine::ConditionVariable cond;
};

struct BackendSharedMutex {
    engine::SharedMutex mutex;
    std::atomic<int> owners{0};
};

}  // namespace

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver_experimental {

MongocGlobalLifecycleScope::MongocGlobalLifecycleScope() {
    UINVARIANT(!engine::current_task::IsTaskProcessorThread(), "MongoDB global lifecycle requires a native stack");
    auto scope = global_lifecycle_scope.Use();
    UINVARIANT(!*scope, "Nested MongoDB global lifecycle scopes");
    *scope = true;
}

MongocGlobalLifecycleScope::~MongocGlobalLifecycleScope() {
    auto scope = global_lifecycle_scope.Use();
    *scope = false;
}

}  // namespace storages::mongo::impl::cdriver_experimental

USERVER_NAMESPACE_END

namespace {

int WaitUntil(BackendConditionVariable& cond, BackendMutex& mutex, engine::Deadline deadline) {
    const engine::TaskCancellationBlocker cancel_blocker;
    std::unique_lock lock(mutex.mutex, std::adopt_lock);
    mutex.MarkUnlocked();
    const auto status = cond.cond.WaitUntil(lock, deadline);
    mutex.MarkLocked();
    lock.release();
    return status == engine::CvStatus::kTimeout ? ETIMEDOUT : 0;
}

}  // namespace

namespace {

// The driver owns the storage and does not promise that it can hold a pointer.
// Keep the coroutine objects separately, keyed by the stable storage address.
template <typename T>
class BackendStorage final {
public:
    void Init(void* storage) {
        Map pending;
        pending.emplace(storage, std::make_shared<T>());
        auto node = pending.extract(storage);
        auto& shard = GetShard(storage);
        const std::lock_guard lock(shard.mutex);
        UINVARIANT(shard.values.insert(std::move(node)).inserted, "MongoDB backend storage initialized twice");
    }

    std::shared_ptr<T> GetOrInit(void* storage) {
        auto& shard = GetShard(storage);
        {
            const std::lock_guard lock(shard.mutex);
            const auto it = shard.values.find(storage);
            if (it != shard.values.end()) {
                return it->second;
            }
        }
        Map pending;
        pending.emplace(storage, std::make_shared<T>());
        auto node = pending.extract(storage);
        const std::lock_guard lock(shard.mutex);
        auto result = shard.values.insert(std::move(node));
        node = std::move(result.node);
        return result.position->second;
    }

    std::shared_ptr<T> Get(void* storage) {
        auto& shard = GetShard(storage);
        const std::lock_guard lock(shard.mutex);
        const auto it = shard.values.find(storage);
        UINVARIANT(it != shard.values.end(), "MongoDB backend storage is not initialized");
        return it->second;
    }

    void Destroy(void* storage) {
        typename Map::node_type node;
        auto& shard = GetShard(storage);
        {
            const std::lock_guard lock(shard.mutex);
            node = shard.values.extract(storage);
            UINVARIANT(!node.empty(), "MongoDB backend storage is not initialized");
        }
    }

private:
    using Map = std::map<void*, std::shared_ptr<T>>;
    struct Shard {
        std::mutex mutex;
        Map values;
    };

    Shard& GetShard(void* storage) {
        return shards_[(reinterpret_cast<std::uintptr_t>(storage) >> kShardAddressShift) % shards_.size()];
    }

    static constexpr std::size_t kShardCount = 64;
    static constexpr unsigned kShardAddressShift = 3;
    std::array<Shard, kShardCount> shards_;
};

BackendStorage<BackendMutex>& MutexStorage() {
    static BackendStorage<BackendMutex> storage;
    return storage;
}

BackendStorage<BackendSharedMutex>& SharedMutexStorage() {
    static BackendStorage<BackendSharedMutex> storage;
    return storage;
}

BackendStorage<BackendConditionVariable>& CondStorage() {
    static BackendStorage<BackendConditionVariable> storage;
    return storage;
}

struct OnceSlot {
    void* state{nullptr};
};

BackendStorage<OnceSlot>& OnceStorage() {
    static BackendStorage<OnceSlot> storage;
    return storage;
}

void BackendOnce(void* storage, void (*callback)()) {
    // Driver once objects have process lifetime. The slot persists until exit.
    auto slot = OnceStorage().GetOrInit(storage);
    auto& state = GetOnceState(&slot->state);
    if (state.finished.load(std::memory_order_acquire)) {
        return;
    }
    const auto initialize = [&] {
        callback();
        state.event.Send();
        state.finished.store(true, std::memory_order_release);
        state.finished.notify_all();
    };
    if (engine::current_task::IsTaskProcessorThread()) {
        const engine::TaskCancellationBlocker cancel_blocker;
        if (!state.started.exchange(true)) {
            initialize();
        } else {
            state.event.Wait();
        }
    } else if (!state.started.exchange(true)) {
        initialize();
    } else {
        state.finished.wait(false, std::memory_order_acquire);
    }
}

int BackendThreadCreate(void* storage, bson_thread_backend_function_t callback, void* arg) {
    if (!storage || !callback || !engine::current_task::IsTaskProcessorThread()) {
        return EINVAL;
    }
    auto handle = std::make_unique<ThreadId>();
    const engine::TaskCancellationBlocker cancel_blocker;
    auto& registry = GetThreadRegistry();
    const std::lock_guard lock(registry.mutex);
    if (registry.next_id == 0) {
        return EAGAIN;
    }
    const auto thread_id = registry.next_id++;
    try {
        auto [it, inserted] = registry.tasks.try_emplace(thread_id);
        UINVARIANT(inserted, "Duplicate MongoDB thread ID");
        it->second = utils::CriticalAsyncBackground(
            fmt::format("mongoc_thread_{}", thread_id),
            engine::current_task::GetTaskProcessor(),
            [callback, arg, thread_id] {
                *current_thread_id = thread_id;
                const engine::TaskCancellationBlocker cancel_blocker;
                callback(arg);
            }
        );
        *handle = thread_id;
        *static_cast<void**>(storage) = handle.release();
        return 0;
    } catch (const std::exception& ex) {
        registry.tasks.erase(thread_id);
        LOG_ERROR() << "Failed to create MongoDB background task: " << ex;
        return EAGAIN;
    }
}

int BackendThreadJoin(void* storage) {
    if (!storage || !engine::current_task::IsTaskProcessorThread()) {
        return EINVAL;
    }
    auto*& handle = *static_cast<ThreadId**>(storage);
    if (!handle) {
        return ESRCH;
    }
    const auto thread_id = *handle;
    if (thread_id == *current_thread_id) {
        return EDEADLK;
    }
    const engine::TaskCancellationBlocker cancel_blocker;
    auto& registry = GetThreadRegistry();
    engine::TaskWithResult<void> task;
    {
        const std::lock_guard lock(registry.mutex);
        const auto it = registry.tasks.find(thread_id);
        if (it == registry.tasks.end()) {
            return ESRCH;
        }
        task = std::move(it->second);
        registry.tasks.erase(it);
    }
    int result = 0;
    try {
        task.Get();
    } catch (const std::exception& ex) {
        LOG_ERROR() << "MongoDB background task failed: " << ex;
        result = EFAULT;
    } catch (...) {
        LOG_ERROR() << "MongoDB background task failed with an unknown exception";
        result = EFAULT;
    }
    delete handle;
    handle = nullptr;
    return result;
}

bool BackendCondIsTimedOut(int ret) { return ret == ETIMEDOUT; }

void BackendSleep(std::int64_t usec) {
    UINVARIANT(usec >= 0, "MongoDB sleep duration must be nonnegative");
    const engine::TaskCancellationBlocker cancel_blocker;
    engine::SleepFor(std::chrono::microseconds{usec});
}

void BackendMutexInit(void* storage) { MutexStorage().Init(storage); }
void BackendMutexDestroy(void* storage) {
    UINVARIANT(!MutexStorage().Get(storage)->locked.load(), "Destroying a locked MongoDB mutex");
    MutexStorage().Destroy(storage);
}
void BackendMutexLock(void* storage) {
    auto mutex = MutexStorage().Get(storage);
    if (!IsGlobalLifecycleScope()) {
        mutex->mutex.lock();
    }
    mutex->MarkLocked();
}
void BackendMutexUnlock(void* storage) {
    auto mutex = MutexStorage().Get(storage);
    mutex->MarkUnlocked();
    if (!IsGlobalLifecycleScope()) {
        mutex->mutex.unlock();
    }
}
bool BackendMutexIsLocked(void* storage) { return MutexStorage().Get(storage)->locked.load(); }

void BackendSharedMutexInit(void* storage) { SharedMutexStorage().Init(storage); }
void BackendSharedMutexDestroy(void* storage) {
    UINVARIANT(SharedMutexStorage().Get(storage)->owners.load() == 0, "Destroying a locked MongoDB shared mutex");
    SharedMutexStorage().Destroy(storage);
}
void BackendSharedMutexLock(void* storage) {
    auto mutex = SharedMutexStorage().Get(storage);
    if (!IsGlobalLifecycleScope()) {
        mutex->mutex.lock();
    }
    UINVARIANT(mutex->owners.exchange(-1) == 0, "Concurrent MongoDB shared mutex lifecycle access");
}
void BackendSharedMutexUnlock(void* storage) {
    auto mutex = SharedMutexStorage().Get(storage);
    UINVARIANT(mutex->owners.exchange(0) == -1, "MongoDB shared mutex is not exclusively locked");
    if (!IsGlobalLifecycleScope()) {
        mutex->mutex.unlock();
    }
}
void BackendSharedMutexLockShared(void* storage) {
    auto mutex = SharedMutexStorage().Get(storage);
    if (!IsGlobalLifecycleScope()) {
        mutex->mutex.lock_shared();
    }
    UINVARIANT(mutex->owners.fetch_add(1) >= 0, "Concurrent MongoDB shared mutex lifecycle access");
}
void BackendSharedMutexUnlockShared(void* storage) {
    auto mutex = SharedMutexStorage().Get(storage);
    UINVARIANT(mutex->owners.fetch_sub(1) > 0, "MongoDB shared mutex is not locked for reading");
    if (!IsGlobalLifecycleScope()) {
        mutex->mutex.unlock_shared();
    }
}

void BackendCondInit(void* storage) { CondStorage().Init(storage); }
void BackendCondDestroy(void* storage) { CondStorage().Destroy(storage); }
int BackendCondWait(void* cond_storage, void* mutex_storage) {
    auto cond = CondStorage().Get(cond_storage);
    auto mutex = MutexStorage().Get(mutex_storage);
    return WaitUntil(*cond, *mutex, {});
}
int BackendCondTimedWait(void* cond_storage, void* mutex_storage, std::int64_t timeout_ms) {
    auto cond = CondStorage().Get(cond_storage);
    auto mutex = MutexStorage().Get(mutex_storage);
    return WaitUntil(*cond, *mutex, engine::Deadline::FromDuration(std::chrono::milliseconds(timeout_ms)));
}
void BackendCondSignal(void* storage) {
    auto cond = CondStorage().Get(storage);
    cond->cond.NotifyOne();
}
void BackendCondBroadcast(void* storage) {
    auto cond = CondStorage().Get(storage);
    cond->cond.NotifyAll();
}

}  // namespace

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver_experimental {

const bson_thread_backend_t& GetThreadBackend() noexcept {
    static const bson_thread_backend_t kBackend{
        .struct_size = sizeof(bson_thread_backend_t),
        .once = BackendOnce,
        .thread_create = BackendThreadCreate,
        .thread_join = BackendThreadJoin,
        .mutex_init = BackendMutexInit,
        .mutex_destroy = BackendMutexDestroy,
        .mutex_lock = BackendMutexLock,
        .mutex_unlock = BackendMutexUnlock,
        .mutex_is_locked = BackendMutexIsLocked,
        .shared_mutex_init = BackendSharedMutexInit,
        .shared_mutex_destroy = BackendSharedMutexDestroy,
        .shared_mutex_lock = BackendSharedMutexLock,
        .shared_mutex_unlock = BackendSharedMutexUnlock,
        .shared_mutex_lock_shared = BackendSharedMutexLockShared,
        .shared_mutex_unlock_shared = BackendSharedMutexUnlockShared,
        .cond_init = BackendCondInit,
        .cond_destroy = BackendCondDestroy,
        .cond_wait = BackendCondWait,
        .cond_timedwait = BackendCondTimedWait,
        .cond_is_timedout = BackendCondIsTimedOut,
        .cond_signal = BackendCondSignal,
        .cond_broadcast = BackendCondBroadcast,
        .sleep = BackendSleep,
    };
    return kBackend;
}

}  // namespace storages::mongo::impl::cdriver_experimental

USERVER_NAMESPACE_END
