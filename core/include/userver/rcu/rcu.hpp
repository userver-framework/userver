#pragma once

/// @file userver/rcu/rcu.hpp
/// @brief @copybrief rcu::Variable

#include <atomic>
#include <cstdlib>
#include <memory>
#include <optional>
#include <utility>

#include <userver/engine/async.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/logging/log.hpp>
#include <userver/rcu/read_indicator.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/clang_format_workarounds.hpp>
#include <userver/utils/impl/wait_token_storage.hpp>

USERVER_NAMESPACE_BEGIN

/// Data structures with MT-access pattern "very often reads, seldom writes"
namespace rcu {

template <typename T>
class Variable;

namespace impl {

template <typename T>
struct SnapshotRecord final {
  std::optional<T> data;
  SnapshotRecord<T>* next_retired{nullptr};
  std::atomic<SnapshotRecord<T>*> next_free{nullptr};
  ReadIndicator indicator;
};

}  // namespace impl

/// Reader smart pointer for rcu::Variable<T>. You may use operator*() or
/// operator->() to do something with the stored value. Once created,
/// ReadablePtr references the same immutable value: if Variable's value is
/// changed during ReadablePtr lifetime, it will not affect value referenced by
/// ReadablePtr.
template <typename T>
class USERVER_NODISCARD ReadablePtr final {
 public:
  explicit ReadablePtr(const Variable<T>& ptr) noexcept {
    auto* record = ptr.current_.load(std::memory_order_relaxed);

    while (true) {
      // Lock 'record', which may or may not be 'current_' by the time we got
      // there.
      lock_ = record->indicator.Lock();

      // Is the record we locked 'current_'? If so, congratulations, we are
      // holding a lock to 'current_'.
      auto* new_current = ptr.current_.load(std::memory_order_acquire);
      if (new_current == record) break;

      // 'current_' changed, try again
      record = new_current;
    }

    ptr_ = &*record->data;
  }

  ReadablePtr(ReadablePtr<T>&& other) noexcept = default;
  ReadablePtr& operator=(ReadablePtr<T>&& other) noexcept = default;
  ReadablePtr(const ReadablePtr<T>& other) noexcept = default;
  ReadablePtr& operator=(const ReadablePtr<T>& other) noexcept = default;
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
    static_assert(!sizeof(T),
                  "Don't use temporary ReadablePtr, store it to a variable");
    std::abort();
  }

  const T* ptr_;
  ReadIndicatorLock lock_;
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
template <typename T>
class USERVER_NODISCARD WritablePtr final {
 public:
  /// For internal use only. Use `var.StartWrite()` instead
  explicit WritablePtr(Variable<T>& var)
      : var_(var), lock_(var.mutex_), record_(var.MakeSnapshot(lock_)) {
    record_->data.emplace(*var.current_.load()->data);
  }

  /// For internal use only. Use `var.Emplace(args...)` instead
  template <typename... Args>
  WritablePtr(Variable<T>& var, std::in_place_t, Args&&... initial_value_args)
      : var_(var), lock_(var.mutex_), record_(var.MakeSnapshot(lock_)) {
    record_->data.emplace(std::forward<Args>(initial_value_args)...);
  }

  WritablePtr(WritablePtr<T>&& other) noexcept
      : var_(other.var_),
        lock_(std::move(other.lock_)),
        record_(std::exchange(other.ptr_, nullptr)) {}

  ~WritablePtr() {
    if (record_) {
      record_->data.reset();
      var_.AppendToFreeList(record_);
    }
  }

  /// Store the changed value in Variable. After Commit() the value becomes
  /// visible to new readers (IOW, Variable::Read() returns ReadablePtr
  /// referencing the stored value, not an old value).
  void Commit() {
    UASSERT(record_ != nullptr);
    var_.DoAssign(std::exchange(record_, nullptr), lock_);
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
  T* GetOnRvalue() {
    static_assert(!sizeof(T),
                  "Don't use temporary WritablePtr, store it to a variable");
    std::abort();
  }

  Variable<T>& var_;
  std::unique_lock<engine::Mutex> lock_;
  impl::SnapshotRecord<T>* record_;
};

/// @brief Can be passed to `rcu::Variable` as the first argument to customize
/// whether old values should be destroyed asynchronously.
enum class DestructionType { kSync, kAsync };

/// @ingroup userver_concurrency userver_containers
///
/// @brief Read-Copy-Update variable
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
/// @see @ref md_en_userver_synchronization
template <typename T>
class Variable final {
 public:
  /// Create a new `Variable` with an in-place constructed initial value.
  /// Asynchronous destruction is enabled by default.
  /// @param initial_value_args arguments passed to the constructor of the
  /// initial value
  template <typename... Args>
  Variable(Args&&... initial_value_args)
      : destruction_type_((std::is_trivially_destructible_v<T> ||
                           std::is_same_v<T, std::string>)
                              ? DestructionType::kSync
                              : DestructionType::kAsync) {
    current_.load(std::memory_order_relaxed)
        ->data.emplace(std::forward<Args>(initial_value_args)...);
  }

  /// Create a new `Variable` with an in-place constructed initial value.
  /// @param destruction_type controls whether destruction of old values should
  /// be performed asynchronously
  /// @param initial_value_args arguments passed to the constructor of the
  /// initial value
  template <typename... Args>
  Variable(DestructionType destruction_type, Args&&... initial_value_args)
      : destruction_type_(destruction_type) {
    current_.load(std::memory_order_relaxed)
        ->data.emplace(std::forward<Args>(initial_value_args)...);
  }

  Variable(const Variable&) = delete;
  Variable(Variable&&) = delete;
  Variable& operator=(const Variable&) = delete;
  Variable& operator=(Variable&&) = delete;

  ~Variable() {
    {
      auto* record = current_.load(std::memory_order_acquire);
      UASSERT_MSG(record->indicator.IsFree(),
                  "RCU variable is destroyed while being used");
      delete record;
    }

    for (auto* record = retire_list_head_; record != nullptr;) {
      auto* next = record->next_retired;
      UASSERT_MSG(record->indicator.IsFree(),
                  "RCU variable is destroyed while being used");
      delete record;
      record = next;
    }

    if (destruction_type_ == DestructionType::kAsync) {
      wait_token_storage_.WaitForAllTokens();
    }

    for (auto* record = free_list_head_.load(std::memory_order_acquire);
         record != nullptr;) {
      auto* next = record->next_free.load(std::memory_order_acquire);
      delete record;
      record = next;
    }
  }

  /// Obtain a smart pointer which can be used to read the current value.
  ReadablePtr<T> Read() const noexcept { return ReadablePtr<T>(*this); }

  /// Obtain a copy of contained value.
  T ReadCopy() const {
    auto ptr = Read();
    return *ptr;
  }

  /// Obtain a smart pointer that will *copy* the current value. The pointer can
  /// be used to make changes to the value and to set the `Variable` to the
  /// changed value.
  WritablePtr<T> StartWrite() { return WritablePtr<T>(*this); }

  /// Obtain a smart pointer to a newly in-place constructed value, but does
  /// not replace the current one yet (in contrast with regular `Emplace`).
  template <typename... Args>
  WritablePtr<T> StartWriteEmplace(Args&&... args) {
    return WritablePtr<T>(*this, std::in_place, std::forward<Args>(args)...);
  }

  /// Replaces the `Variable`'s value with the provided one.
  void Assign(T new_value) {
    WritablePtr<T>(*this, std::in_place, std::move(new_value)).Commit();
  }

  /// Replaces the `Variable`'s value with an in-place constructed one.
  template <typename... Args>
  void Emplace(Args&&... args) {
    WritablePtr<T>(*this, std::in_place, std::forward<Args>(args)...).Commit();
  }

  void Cleanup() {
    std::unique_lock lock(mutex_, std::try_to_lock);
    if (!lock.owns_lock()) {
      // Someone is already assigning to the RCU. They will call ScanRetireList
      // in the process.
      return;
    }
    ScanRetireList(lock);
  }

 private:
  void DoAssign(impl::SnapshotRecord<T>* new_snapshot,
                std::unique_lock<engine::Mutex>& lock) {
    UASSERT(lock.owns_lock());
    ScanRetireList(lock);
    auto* old_snapshot = current_.load(std::memory_order_relaxed);
    current_.store(new_snapshot, std::memory_order_release);
    RetireSnapshot(old_snapshot, lock);
  }

  [[nodiscard]] impl::SnapshotRecord<T>* MakeSnapshot(
      std::unique_lock<engine::Mutex>& lock) {
    UASSERT(lock.owns_lock());
    auto* record = RemoveFromFreeList(lock);
    return record ? record : new impl::SnapshotRecord<T>;
  }

  void ScanRetireList(std::unique_lock<engine::Mutex>& lock) noexcept {
    UASSERT(lock.owns_lock());

    auto** next = &retire_list_head_;

    while (*next != nullptr) {
      if ((*next)->indicator.IsFree()) {
        auto* retired_record = *next;
        *next = retired_record->next_retired;
        retired_record->next_retired = nullptr;
        DeleteSnapshot(retired_record);
      } else {
        next = &(*next)->next_retired;
      }
    }
  }

  void RetireSnapshot(impl::SnapshotRecord<T>* record,
                      std::unique_lock<engine::Mutex>& lock) {
    UASSERT(lock.owns_lock());

    if (record->indicator.IsFree()) {
      DeleteSnapshot(record);
    } else {
      record->next_retired = retire_list_head_;
      retire_list_head_ = record;
    }
  }

  void DeleteSnapshot(impl::SnapshotRecord<T>* record) {
    UASSERT(record != nullptr);

    switch (destruction_type_) {
      case DestructionType::kSync:
        record->data.reset();
        AppendToFreeList(record);
        break;
      case DestructionType::kAsync:
        engine::CriticalAsyncNoSpan([this, record,
                                     token = wait_token_storage_.GetToken()] {
          record->data.reset();
          AppendToFreeList(record);
        }).Detach();
        break;
    }
  }

  void AppendToFreeList(impl::SnapshotRecord<T>* record) {
    UASSERT(record != nullptr);

    // Treiber stack algorithm is used.
    auto* old_head = free_list_head_.load(std::memory_order_relaxed);
    do {
      record->next_free.store(old_head, std::memory_order_relaxed);
    } while (!free_list_head_.compare_exchange_weak(old_head, record,
                                                    std::memory_order_release,
                                                    std::memory_order_relaxed));
  }

  [[nodiscard]] impl::SnapshotRecord<T>* RemoveFromFreeList(
      std::unique_lock<engine::Mutex>& lock) {
    UASSERT(lock.owns_lock());

    impl::SnapshotRecord<T>* old_head{};
    impl::SnapshotRecord<T>* next{};

    // Treiber stack algorithm is used.
    // ABA situation cannot occur, because popping from the stack is
    // performed under the writer lock (a node can't get popped and pushed
    // back in parallel with us).
    do {
      old_head = free_list_head_.load(std::memory_order_acquire);
      if (old_head == nullptr) return nullptr;
      next = old_head->next_free.load(std::memory_order_relaxed);
    } while (!free_list_head_.compare_exchange_weak(
        old_head, next, std::memory_order_acquire, std::memory_order_relaxed));

    return old_head;
  }

 private:
  const DestructionType destruction_type_;
  std::atomic<impl::SnapshotRecord<T>*> current_{new impl::SnapshotRecord<T>};
  std::atomic<impl::SnapshotRecord<T>*> free_list_head_{nullptr};

  // covers everything except current_ reads and free_list_head_
  engine::Mutex mutex_;
  impl::SnapshotRecord<T>* retire_list_head_{nullptr};
  utils::impl::WaitTokenStorage wait_token_storage_;

  friend class ReadablePtr<T>;
  friend class WritablePtr<T>;
};

}  // namespace rcu

USERVER_NAMESPACE_END
