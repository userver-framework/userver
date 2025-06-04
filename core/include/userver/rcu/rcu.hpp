#pragma once

/// @file userver/rcu/rcu.hpp
/// @brief @copybrief rcu::Variable

#include <atomic>
#include <cstdlib>
#include <mutex>
#include <optional>
#include <utility>

#include <userver/concurrent/impl/asymmetric_fence.hpp>
#include <userver/concurrent/impl/intrusive_hooks.hpp>
#include <userver/concurrent/impl/intrusive_stack.hpp>
#include <userver/concurrent/impl/striped_read_indicator.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/rcu/fwd.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/wait_token_storage.hpp>

USERVER_NAMESPACE_BEGIN

/// @brief Read-Copy-Update
///
/// @see Based on ideas from
/// http://www.drdobbs.com/lock-free-data-structures-with-hazard-po/184401890
/// with modified API
namespace rcu {

namespace impl {

template <typename T>
struct SnapshotRecord final {
    std::optional<T> data;
    concurrent::impl::StripedReadIndicator indicator;
    concurrent::impl::SinglyLinkedHook<SnapshotRecord> free_list_hook;
    SnapshotRecord* next_retired{nullptr};
};

// Used instead of concurrent::impl::MemberHook to avoid instantiating
// SnapshotRecord<T> ahead of time.
template <typename T>
struct FreeListHookGetter {
    auto& operator()(SnapshotRecord<T>& node) const noexcept { return node.free_list_hook; }
};

template <typename T>
struct SnapshotRecordFreeList {
    SnapshotRecordFreeList() = default;

    ~SnapshotRecordFreeList() {
        list.DisposeUnsafe([](SnapshotRecord<T>& record) { delete &record; });
    }

    concurrent::impl::IntrusiveStack<SnapshotRecord<T>, FreeListHookGetter<T>> list;
};

template <typename T>
class SnapshotRecordRetiredList final {
public:
    SnapshotRecordRetiredList() = default;

    bool IsEmpty() const noexcept { return head_ == nullptr; }

    void Push(SnapshotRecord<T>& record) noexcept {
        record.next_retired = head_;
        head_ = &record;
    }

    template <typename Predicate, typename Disposer>
    void RemoveAndDisposeIf(Predicate predicate, Disposer disposer) {
        SnapshotRecord<T>** ptr_to_current = &head_;

        while (*ptr_to_current != nullptr) {
            SnapshotRecord<T>* const current = *ptr_to_current;

            if (predicate(*current)) {
                *ptr_to_current = std::exchange(current->next_retired, nullptr);
                disposer(*current);
            } else {
                ptr_to_current = &current->next_retired;
            }
        }
    }

private:
    SnapshotRecord<T>* head_{nullptr};
};

class ExclusiveMutex final {
public:
    void lock() {
        const bool was_locked = is_locked_.exchange(true);
        UINVARIANT(
            !was_locked,
            "Detected a race condition when multiple writers Assign to an rcu::Variable concurrently. The value that "
            "will remain in rcu::Variable when the dust settles is unspecified."
        );
    }

    void unlock() noexcept { is_locked_.store(false); }

private:
    std::atomic<bool> is_locked_{false};
};

}  // namespace impl

/// @brief A handle to the retired object version, which an RCU deleter should
/// clean up.
/// @see rcu::DefaultRcuTraits
template <typename T>
class SnapshotHandle final {
public:
    SnapshotHandle(SnapshotHandle&& other) noexcept
        : record_(std::exchange(other.record_, nullptr)), free_list_(std::exchange(other.free_list_, nullptr)) {}

    ~SnapshotHandle() {
        if (record_ != nullptr) {
            UASSERT(free_list_ != nullptr);
            record_->data.reset();
            free_list_->list.Push(*record_);
        }
    }

private:
    template <typename /*T*/, typename Traits>
    friend class Variable;

    template <typename /*T*/, typename Traits>
    friend class WritablePtr;

    explicit SnapshotHandle(impl::SnapshotRecord<T>& record, impl::SnapshotRecordFreeList<T>& free_list) noexcept
        : record_(&record), free_list_(&free_list) {}

    impl::SnapshotRecord<T>* record_;
    impl::SnapshotRecordFreeList<T>* free_list_;
};

/// @brief Destroys retired objects synchronously.
/// @see rcu::DefaultRcuTraits
struct SyncDeleter final {
    template <typename T>
    void Delete(SnapshotHandle<T>&& handle) noexcept {
        [[maybe_unused]] const auto for_deletion = std::move(handle);
    }
};

/// @brief Destroys retired objects asynchronously in the same `TaskProcessor`.
/// @see rcu::DefaultRcuTraits
class AsyncDeleter final {
public:
    ~AsyncDeleter() { wait_token_storage_.WaitForAllTokens(); }

    template <typename T>
    void Delete(SnapshotHandle<T>&& handle) noexcept {
        if constexpr (std::is_trivially_destructible_v<T> || std::is_same_v<T, std::string>) {
            SyncDeleter{}.Delete(std::move(handle));
        } else {
            try {
                engine::DetachUnscopedUnsafe(engine::CriticalAsyncNoSpan(
                    // The order of captures is important, 'handle' must be destroyed before 'token'.
                    [token = wait_token_storage_.GetToken(), handle = std::move(handle)]() mutable {}
                ));
                // NOLINTNEXTLINE(bugprone-empty-catch)
            } catch (...) {
                // Task creation somehow failed.
                // `handle` will be destroyed synchronously, because it is already moved
                // into the task's lambda.
            }
        }
    }

private:
    utils::impl::WaitTokenStorage wait_token_storage_;
};

/// @brief Default RCU traits. Deletes garbage asynchronously.
/// Designed for storing data of multi-megabyte or multi-gigabyte caches.
/// @note Allows reads from any kind of thread.
/// Only allows writes from coroutine threads.
/// @see rcu::Variable
/// @see rcu::SyncRcuTraits
/// @see rcu::BlockingRcuTraits
struct DefaultRcuTraits {
    /// `MutexType` is a writer's mutex type that has to be used to protect
    /// structure on update.
    using MutexType = engine::Mutex;

    /// `DeleterType` is used to delete retired objects. It should:
    /// 1. should contain `void Delete(SnapshotHandle<T>) noexcept`;
    /// 2. force synchronous cleanup of remaining handles on destruction.
    using DeleterType = AsyncDeleter;
};

/// @brief Deletes garbage synchronously.
/// Designed for storing small amounts of data with relatively fast destructors.
/// @note Allows reads from any kind of thread.
/// Only allows writes from coroutine threads.
/// @see rcu::DefaultRcuTraits
struct SyncRcuTraits : public DefaultRcuTraits {
    using DeleterType = SyncDeleter;
};

/// @brief Rcu traits for using outside of coroutines.
/// @note Allows reads from any kind of thread.
/// Only allows writes from NON-coroutine threads.
/// @warning Blocks writing threads which are coroutines, which can cause
/// deadlocks and hangups.
/// @see rcu::DefaultRcuTraits
struct BlockingRcuTraits : public DefaultRcuTraits {
    using MutexType = std::mutex;
    using DeleterType = SyncDeleter;
};

/// @brief Rcu traits that only allow a single writer.
/// Detects race conditions when multiple writers call `Assign` concurrently.
/// @note Allows reads and writes from any kind of thread.
/// @see rcu::DefaultRcuTraits
struct ExclusiveRcuTraits : public DefaultRcuTraits {
    using MutexType = impl::ExclusiveMutex;
    using DeleterType = SyncDeleter;
};

/// Reader smart pointer for rcu::Variable<T>. You may use operator*() or
/// operator->() to do something with the stored value. Once created,
/// ReadablePtr references the same immutable value: if Variable's value is
/// changed during ReadablePtr lifetime, it will not affect value referenced by
/// ReadablePtr.
template <typename T, typename RcuTraits>
class [[nodiscard]] ReadablePtr final {
public:
    explicit ReadablePtr(const Variable<T, RcuTraits>& ptr) {
        auto* record = ptr.current_.load();

        while (true) {
            // Lock 'record', which may or may not be 'current_' by the time we got
            // there.
            lock_ = record->indicator.Lock();

            // seq_cst is required for indicator.Lock in the following case.
            //
            // Reader thread point-of-view:
            // 1. [reader] load current_
            // 2. [reader] indicator.Lock
            // 3. [reader] load current_
            // 4. [writer] store current_
            // 5. [writer] indicator.IsFree
            //
            // Given seq_cst only on (3), (4), and (5), the writer can see
            // (2) after (5). In this case the reader will think that it has
            // successfully taken the lock, which is false.
            //
            // So we need seq_cst on all of (2), (3), (4), (5). Making (2) seq_cst is
            // somewhat expensive, but this is a well-known cost of hazard pointers.
            //
            // The seq_cst cost can be mitigated by utilizing asymmetric fences.
            // This asymmetric fence effectively grants std::memory_order_seq_cst
            // to indicator.Lock when applied together with AsymmetricThreadFenceHeavy
            // in (5). The technique is taken from Folly HazPtr.
            concurrent::impl::AsymmetricThreadFenceLight();

            // Is the record we locked 'current_'? If so, congratulations, we are
            // holding a lock to 'current_'.
            auto* new_current = ptr.current_.load(std::memory_order_seq_cst);
            if (new_current == record) break;

            // 'current_' changed, try again
            record = new_current;
        }

        ptr_ = &*record->data;
    }

    ReadablePtr(ReadablePtr&& other) noexcept = default;
    ReadablePtr& operator=(ReadablePtr&& other) noexcept = default;
    ReadablePtr(const ReadablePtr& other) = default;
    ReadablePtr& operator=(const ReadablePtr& other) = default;
    ~ReadablePtr() = default;

    const T* Get() const& {
        UASSERT(ptr_);
        return ptr_;
    }

    const T* Get() && { return GetOnRvalue(); }

    const T* operator->() const& { return Get(); }
    const T* operator->() && { return GetOnRvalue(); }

    const T& operator*() const& { return *Get(); }
    const T& operator*() && { return *GetOnRvalue(); }

private:
    const T* GetOnRvalue() {
        static_assert(!sizeof(T), "Don't use temporary ReadablePtr, store it to a variable");
        std::abort();
    }

    const T* ptr_;
    concurrent::impl::StripedReadIndicatorLock lock_;
};

/// Smart pointer for rcu::Variable<T> for changing RCU value. It stores a
/// reference to a to-be-changed value and allows one to mutate the value (e.g.
/// add items to std::unordered_map). Changed value is not visible to readers
/// until explicit store by Commit. Only a single writer may own a WritablePtr
/// associated with the same Variable, so WritablePtr creates a critical
/// section. This critical section doesn't affect readers, so a slow writer
/// doesn't block readers.
/// @note you may not pass WritablePtr between coroutines as it owns
/// engine::Mutex, which must be unlocked in the same coroutine that was used to
/// lock the mutex.
template <typename T, typename RcuTraits>
class [[nodiscard]] WritablePtr final {
public:
    /// @cond
    // For internal use only. Use `var.StartWrite()` instead
    explicit WritablePtr(Variable<T, RcuTraits>& var)
        : var_(var), lock_(var.mutex_), record_(&var.EmplaceSnapshot(*var.current_.load()->data)) {}

    // For internal use only. Use `var.Emplace(args...)` instead
    template <typename... Args>
    WritablePtr(Variable<T, RcuTraits>& var, std::in_place_t, Args&&... initial_value_args)
        : var_(var), lock_(var.mutex_), record_(&var.EmplaceSnapshot(std::forward<Args>(initial_value_args)...)) {}
    /// @endcond

    WritablePtr(WritablePtr&& other) noexcept
        : var_(other.var_), lock_(std::move(other.lock_)), record_(std::exchange(other.record_, nullptr)) {}

    ~WritablePtr() {
        if (record_) {
            var_.DeleteSnapshot(*record_);
        }
    }

    /// Store the changed value in Variable. After Commit() the value becomes
    /// visible to new readers (IOW, Variable::Read() returns ReadablePtr
    /// referencing the stored value, not an old value).
    void Commit() {
        UASSERT(record_ != nullptr);
        var_.DoAssign(*std::exchange(record_, nullptr), lock_);
        lock_.unlock();
    }

    T* Get() & {
        UASSERT(record_ != nullptr);
        return &*record_->data;
    }

    T* Get() && { return GetOnRvalue(); }

    T* operator->() & { return Get(); }
    T* operator->() && { return GetOnRvalue(); }

    T& operator*() & { return *Get(); }
    T& operator*() && { return *GetOnRvalue(); }

private:
    [[noreturn]] static T* GetOnRvalue() {
        static_assert(!sizeof(T), "Don't use temporary WritablePtr, store it to a variable");
        std::abort();
    }

    Variable<T, RcuTraits>& var_;
    std::unique_lock<typename RcuTraits::MutexType> lock_;
    impl::SnapshotRecord<T>* record_;
};

/// @ingroup userver_concurrency userver_containers
///
/// @brief Read-Copy-Update variable
///
/// @see Based on ideas from
/// http://www.drdobbs.com/lock-free-data-structures-with-hazard-po/184401890
/// with modified API.
///
/// A variable with MT-access pattern "very often reads, seldom writes". It is
/// specially optimized for reads. On read, one obtains a ReaderPtr<T> from it
/// and uses the obtained value as long as it wants to. On write, one obtains a
/// WritablePtr<T> with a copy of the last version of the value, makes some
/// changes to it, and commits the result to update current variable value (does
/// Read-Copy-Update). Old version of the value is not freed on update, it will
/// be eventually freed when a subsequent writer identifies that nobody works
/// with this version.
///
/// @note There is no way to create a "null" `Variable`.
///
/// ## Example usage:
///
/// @snippet rcu/rcu_test.cpp  Sample rcu::Variable usage
///
/// @see @ref scripts/docs/en/userver/synchronization.md
///
/// @tparam T the stored value
/// @tparam RcuTraits traits, should inherit from rcu::DefaultRcuTraits
template <typename T, typename RcuTraits>
class Variable final {
    static_assert(
        std::is_base_of_v<DefaultRcuTraits, RcuTraits>,
        "RcuTraits should publicly inherit from rcu::DefaultRcuTraits"
    );

public:
    using MutexType = typename RcuTraits::MutexType;
    using DeleterType = typename RcuTraits::DeleterType;

    /// @brief Create a new `Variable` with an in-place constructed initial value.
    /// @param initial_value_args arguments passed to the constructor of the
    /// initial value
    template <typename... Args>
    // TODO make explicit
    Variable(Args&&... initial_value_args) : current_(&EmplaceSnapshot(std::forward<Args>(initial_value_args)...)) {}

    Variable(const Variable&) = delete;
    Variable(Variable&&) = delete;
    Variable& operator=(const Variable&) = delete;
    Variable& operator=(Variable&&) = delete;

    ~Variable() {
        {
            auto* record = current_.load();
            UASSERT_MSG(record->indicator.IsFree(), "RCU variable is destroyed while being used");
            delete record;
        }

        retired_list_.RemoveAndDisposeIf(
            [](impl::SnapshotRecord<T>&) { return true; },
            [](impl::SnapshotRecord<T>& record) {
                UASSERT_MSG(record.indicator.IsFree(), "RCU variable is destroyed while being used");
                delete &record;
            }
        );
    }

    /// Obtain a smart pointer which can be used to read the current value.
    ReadablePtr<T, RcuTraits> Read() const { return ReadablePtr<T, RcuTraits>(*this); }

    /// Obtain a copy of contained value.
    T ReadCopy() const {
        auto ptr = Read();
        return *ptr;
    }

    /// Obtain a smart pointer that will *copy* the current value. The pointer can
    /// be used to make changes to the value and to set the `Variable` to the
    /// changed value.
    WritablePtr<T, RcuTraits> StartWrite() { return WritablePtr<T, RcuTraits>(*this); }

    /// Obtain a smart pointer to a newly in-place constructed value, but does
    /// not replace the current one yet (in contrast with regular `Emplace`).
    template <typename... Args>
    WritablePtr<T, RcuTraits> StartWriteEmplace(Args&&... args) {
        return WritablePtr<T, RcuTraits>(*this, std::in_place, std::forward<Args>(args)...);
    }

    /// Replaces the `Variable`'s value with the provided one.
    void Assign(T new_value) { WritablePtr<T, RcuTraits>(*this, std::in_place, std::move(new_value)).Commit(); }

    /// Replaces the `Variable`'s value with an in-place constructed one.
    template <typename... Args>
    void Emplace(Args&&... args) {
        WritablePtr<T, RcuTraits>(*this, std::in_place, std::forward<Args>(args)...).Commit();
    }

    void Cleanup() {
        std::unique_lock lock(mutex_, std::try_to_lock);
        if (!lock.owns_lock()) {
            // Someone is already assigning to the RCU. They will call ScanRetireList
            // in the process.
            return;
        }
        ScanRetiredList(lock);
    }

private:
    friend class ReadablePtr<T, RcuTraits>;
    friend class WritablePtr<T, RcuTraits>;

    void DoAssign(impl::SnapshotRecord<T>& new_snapshot, std::unique_lock<MutexType>& lock) {
        UASSERT(lock.owns_lock());

        // Note: exchange RMW operation would not give any benefits here.
        auto* const old_snapshot = current_.load();
        current_.store(&new_snapshot, std::memory_order_seq_cst);

        UASSERT(old_snapshot);
        retired_list_.Push(*old_snapshot);
        ScanRetiredList(lock);
    }

    template <typename... Args>
    [[nodiscard]] impl::SnapshotRecord<T>& EmplaceSnapshot(Args&&... args) {
        auto* const free_list_record = free_list_.list.TryPop();
        auto& record = free_list_record ? *free_list_record : *new impl::SnapshotRecord<T>{};
        UASSERT(!record.data);

        try {
            record.data.emplace(std::forward<Args>(args)...);
        } catch (...) {
            free_list_.list.Push(record);
            throw;
        }

        return record;
    }

    void ScanRetiredList(std::unique_lock<MutexType>& lock) noexcept {
        UASSERT(lock.owns_lock());
        if (retired_list_.IsEmpty()) return;

        concurrent::impl::AsymmetricThreadFenceHeavy();

        retired_list_.RemoveAndDisposeIf(
            [](impl::SnapshotRecord<T>& record) { return record.indicator.IsFree(); },
            [&](impl::SnapshotRecord<T>& record) { DeleteSnapshot(record); }
        );
    }

    void DeleteSnapshot(impl::SnapshotRecord<T>& record) noexcept {
        static_assert(
            noexcept(deleter_.Delete(SnapshotHandle<T>{record, free_list_})), "DeleterType::Delete must be noexcept"
        );
        deleter_.Delete(SnapshotHandle<T>{record, free_list_});
    }

    // Covers current_ writes, free_list_.Pop, retired_list_
    MutexType mutex_{};
    impl::SnapshotRecordFreeList<T> free_list_;
    impl::SnapshotRecordRetiredList<T> retired_list_;
    // Must be placed after 'free_list_' to force sync cleanup before
    // the destruction of free_list_.
    DeleterType deleter_{};
    // Must be placed after 'free_list_' and 'deleter_' so that if
    // the initialization of current_ throws, it can be disposed properly.
    std::atomic<impl::SnapshotRecord<T>*> current_;
};

}  // namespace rcu

USERVER_NAMESPACE_END
