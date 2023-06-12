#pragma once

/// @file userver/rcu/rcu.hpp
/// @brief Implementation of hazard pointer

#include <atomic>
#include <cstdlib>
#include <list>
#include <unordered_set>

#include <userver/engine/async.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/logging/log.hpp>
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

// Hazard pointer implementation. Pointers form a linked list. \p ptr points
// to the data they 'hold', next - to the next element in a list.
// kUsed is a filler value to show that hazard pointer is not free. Please see
// comment to it. Please note, pointers do not form a linked list with
// application-wide list, that is there is no 'static
// std::atomic<HazardPointerRecord*> global_head' Every rcu::Variable has its
// own list of hazard pointers. Thus, move-assignment on hazard pointers is
// difficult to implement.
template <typename T, typename RcuTraits>
struct HazardPointerRecord final {
  // You see, objects are created 'filled', that is for the purposes of hazard
  // pointer list, they contain value. This eliminates some race conditions,
  // because these algorithms checks for ptr != nullptr (And kUsed is not
  // nullptr). Obviously, this value can't be used, when dereferenced it points
  // somewhere into kernel space and will cause SEGFAULT
  static inline T* const kUsed = reinterpret_cast<T*>(1);

  explicit HazardPointerRecord(const Variable<T, RcuTraits>& owner)
      : owner(owner) {}

  std::atomic<T*> ptr = kUsed;
  const Variable<T, RcuTraits>& owner;
  std::atomic<HazardPointerRecord*> next{nullptr};

  // Simple operation that marks this hazard pointer as no longer used.
  void Release() { ptr = nullptr; }
};

template <typename T, typename RcuTraits>
struct CachedData {
  impl::HazardPointerRecord<T, RcuTraits>* hp{nullptr};
  const Variable<T, RcuTraits>* variable{nullptr};
  // ensures that `variable` points to the instance that filled the cache
  uint64_t variable_epoch{0};
};

template <typename T, typename RcuTraits>
thread_local CachedData<T, RcuTraits> cache;

uint64_t GetNextEpoch() noexcept;

}  // namespace impl

/// Default Rcu traits.
/// - `MutexType` is a writer's mutex type that has to be used to protect
/// structure on update
template <typename T>
struct DefaultRcuTraits {
  using MutexType = engine::Mutex;
};

/// Reader smart pointer for rcu::Variable<T>. You may use operator*() or
/// operator->() to do something with the stored value. Once created,
/// ReadablePtr references the same immutable value: if Variable's value is
/// changed during ReadablePtr lifetime, it will not affect value referenced by
/// ReadablePtr.
template <typename T, typename RcuTraits>
class [[nodiscard]] ReadablePtr final {
 public:
  explicit ReadablePtr(const Variable<T, RcuTraits>& ptr)
      : hp_record_(&ptr.MakeHazardPointer()) {
    // This cycle guarantees that at the end of it both t_ptr_ and
    // hp_record_->ptr will both be set to
    // 1. something meaningful
    // 2. and that this meaningful value was not removed between assigning to
    //    t_ptr_ and storing  it in a hazard pointer
    do {
      t_ptr_ = ptr.GetCurrent();

      hp_record_->ptr.store(t_ptr_);
    } while (t_ptr_ != ptr.GetCurrent());
  }

  ReadablePtr(ReadablePtr<T, RcuTraits>&& other) noexcept
      : t_ptr_(other.t_ptr_), hp_record_(other.hp_record_) {
    other.t_ptr_ = nullptr;
  }

  ReadablePtr& operator=(ReadablePtr<T, RcuTraits>&& other) noexcept {
    // What do we have here?
    // 1. 'other' may point to the same variable - or to a different one.
    // 2. therefore, its hazard pointer may belong to the same list,
    //    or to a different one.

    // We can't move hazard pointers between different lists - there is simply
    // no way to do this.
    // We also can't simply swap two pointers inside hazard pointers and leave
    // it be, because if 'other' is from different Variable, then our
    // ScanRetiredList won't find any more pointer to current T* and will
    // destroy object, while 'other' is still holding a hazard pointer to it.
    // The thing is, whole ReadablePtr is just a glorified holder for
    // hazard pointer + t_ptr_ just so that we don't have to load() atomics
    // every time.
    // so, lets just take pointer to hazard pointer inside other
    UASSERT_MSG(this != &other, "Self assignment to RCU variable");
    if (this == &other) {
      // it is an error to assign to self. Do nothing in this case
      return *this;
    }

    // Get rid of our current hp_record_
    if (t_ptr_) {
      hp_record_->Release();
    }
    // After that moment, the content of our hp_record_ can't be used -
    // no more hp_record_->xyz calls, because it is probably already reused in
    // some other ReadablePtr. Also, don't call t_ptr_, it is probably already
    // freed. Just take values from 'other'.
    hp_record_ = other.hp_record_;
    t_ptr_ = other.t_ptr_;

    // Now, it won't do us any good if there were two glorified things having
    // pointer to same hp_record_. Kill the other one.
    other.t_ptr_ = nullptr;
    // We don't need to clean other.hp_record_, because other.t_ptr_ acts
    // like a guard to it. As long as other.t_ptr_ is nullptr, nobody will
    // use other.hp_record_

    return *this;
  }

  ReadablePtr(const ReadablePtr<T, RcuTraits>& other)
      : ReadablePtr(other.hp_record_->owner) {}

  ReadablePtr& operator=(const ReadablePtr<T, RcuTraits>& other) {
    if (this != &other) *this = ReadablePtr<T, RcuTraits>{other};
    return *this;
  }

  ~ReadablePtr() {
    if (!t_ptr_) return;
    UASSERT(hp_record_ != nullptr);
    hp_record_->Release();
  }

  const T* Get() const& {
    UASSERT(t_ptr_);
    return t_ptr_;
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

  // This is a pointer to actual data. If it is null, then we treat it as
  // an indicator that this ReadablePtr is cleared and won't call
  // any logic associated with hp_record_
  T* t_ptr_;
  // Our hazard pointer. It can be nullptr in some circumstances.
  // Invariant is this: if t_ptr_ is not nullptr, then hp_record_ is also
  // not nullptr and points to hazard pointer containing same T*.
  // Thus, if t_ptr_ is nullptr, then hp_record_ is undefined.
  impl::HazardPointerRecord<T, RcuTraits>* hp_record_;
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
  /// For internal use only. Use `var.StartWrite()` instead
  explicit WritablePtr(Variable<T, RcuTraits>& var)
      : var_(var),
        lock_(var.mutex_),
        ptr_(std::make_unique<T>(*var_.GetCurrent())) {
    LOG_TRACE() << "Start writing ptr=" << ptr_.get();
  }

  /// For internal use only. Use `var.Emplace(args...)` instead
  template <typename... Args>
  WritablePtr(Variable<T, RcuTraits>& var, std::in_place_t,
              Args&&... initial_value_args)
      : var_(var),
        lock_(var.mutex_),
        ptr_(std::make_unique<T>(std::forward<Args>(initial_value_args)...)) {
    LOG_TRACE() << "Start writing ptr=" << ptr_.get()
                << " with custom initial value";
  }

  WritablePtr(WritablePtr<T, RcuTraits>&& other) noexcept
      : var_(other.var_),
        lock_(std::move(other.lock_)),
        ptr_(std::move(other.ptr_)) {
    LOG_TRACE() << "Continue writing ptr=" << ptr_.get();
  }

  ~WritablePtr() {
    if (ptr_) {
      LOG_TRACE() << "Stop writing ptr=" << ptr_.get();
    }
  }

  /// Store the changed value in Variable. After Commit() the value becomes
  /// visible to new readers (IOW, Variable::Read() returns ReadablePtr
  /// referencing the stored value, not an old value).
  void Commit() {
    UASSERT(ptr_ != nullptr);
    LOG_TRACE() << "Committing ptr=" << ptr_.get();

    std::unique_ptr<T> old_ptr(var_.current_.exchange(ptr_.release()));
    var_.Retire(std::move(old_ptr), lock_);
    lock_.unlock();
  }

  T* Get() & {
    UASSERT(ptr_);
    return ptr_.get();
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

  Variable<T, RcuTraits>& var_;
  std::unique_lock<typename RcuTraits::MutexType> lock_;
  std::unique_ptr<T> ptr_;
};

/// @brief Can be passed to `rcu::Variable` as the first argument to customize
/// whether old values should be destroyed asynchronously.
enum class DestructionType { kSync, kAsync };

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
/// @see @ref md_en_userver_synchronization
template <typename T, typename RcuTraits>
class Variable final {
 public:
  using MutexType = typename RcuTraits::MutexType;

  /// Create a new `Variable` with an in-place constructed initial value.
  /// Asynchronous destruction is enabled by default.
  /// @param initial_value_args arguments passed to the constructor of the
  /// initial value
  template <typename... Args>
  Variable(Args&&... initial_value_args)
      : destruction_type_(std::is_trivially_destructible_v<T> ||
                                  std::is_same_v<T, std::string> ||
                                  !std::is_same_v<MutexType, engine::Mutex>
                              ? DestructionType::kSync
                              : DestructionType::kAsync),
        epoch_(impl::GetNextEpoch()),
        current_(new T(std::forward<Args>(initial_value_args)...)) {}

  /// Create a new `Variable` with an in-place constructed initial value.
  /// @param destruction_type controls whether destruction of old values should
  /// be performed asynchronously
  /// @param initial_value_args arguments passed to the constructor of the
  /// initial value
  template <typename... Args>
  Variable(DestructionType destruction_type, Args&&... initial_value_args)
      : destruction_type_(destruction_type),
        epoch_(impl::GetNextEpoch()),
        current_(new T(std::forward<Args>(initial_value_args)...)) {}

  Variable(const Variable&) = delete;
  Variable(Variable&&) = delete;
  Variable& operator=(const Variable&) = delete;
  Variable& operator=(Variable&&) = delete;

  ~Variable() {
    delete current_.load();

    auto* hp = hp_record_head_.load();
    while (hp) {
      auto* next = hp->next.load();
      UASSERT_MSG(hp->ptr == nullptr,
                  "RCU variable is destroyed while being used");
      delete hp;
      hp = next;
    }

    // Make sure all data is deleted after return from dtr
    if (destruction_type_ == DestructionType::kAsync) {
      wait_token_storage_.WaitForAllTokens();
    }
  }

  /// Obtain a smart pointer which can be used to read the current value.
  ReadablePtr<T, RcuTraits> Read() const {
    return ReadablePtr<T, RcuTraits>(*this);
  }

  /// Obtain a copy of contained value.
  T ReadCopy() const {
    auto ptr = Read();
    return *ptr;
  }

  /// Obtain a smart pointer that will *copy* the current value. The pointer can
  /// be used to make changes to the value and to set the `Variable` to the
  /// changed value.
  WritablePtr<T, RcuTraits> StartWrite() {
    return WritablePtr<T, RcuTraits>(*this);
  }

  /// Obtain a smart pointer to a newly in-place constructed value, but does
  /// not replace the current one yet (in contrast with regular `Emplace`).
  template <typename... Args>
  WritablePtr<T, RcuTraits> StartWriteEmplace(Args&&... args) {
    return WritablePtr<T, RcuTraits>(*this, std::in_place,
                                     std::forward<Args>(args)...);
  }

  /// Replaces the `Variable`'s value with the provided one.
  void Assign(T new_value) {
    WritablePtr<T, RcuTraits>(*this, std::in_place, std::move(new_value))
        .Commit();
  }

  /// Replaces the `Variable`'s value with an in-place constructed one.
  template <typename... Args>
  void Emplace(Args&&... args) {
    WritablePtr<T, RcuTraits>(*this, std::in_place, std::forward<Args>(args)...)
        .Commit();
  }

  void Cleanup() {
    std::unique_lock lock(mutex_, std::try_to_lock);
    if (!lock.owns_lock()) {
      LOG_TRACE() << "Not cleaning up, someone else is holding the mutex lock";
      // Someone is already changing the RCU
      return;
    }

    ScanRetiredList(CollectHazardPtrs(lock));
  }

 private:
  T* GetCurrent() const { return current_.load(); }

  impl::HazardPointerRecord<T, RcuTraits>* MakeHazardPointerCached() const {
    auto& cache = impl::cache<T, RcuTraits>;
    auto* hp = cache.hp;
    T* ptr = nullptr;
    if (hp && cache.variable == this && cache.variable_epoch == epoch_) {
      if (hp->ptr.load() == nullptr &&
          hp->ptr.compare_exchange_strong(
              ptr, impl::HazardPointerRecord<T, RcuTraits>::kUsed)) {
        return hp;
      }
    }

    return nullptr;
  }

  impl::HazardPointerRecord<T, RcuTraits>* MakeHazardPointerFast() const {
    // Look for any hazard pointer with nullptr data ptr.
    // Mark it with kUsed (to reserve it for ourselves) and return it.
    auto* hp = hp_record_head_.load();
    while (hp) {
      T* t_ptr = nullptr;
      if (hp->ptr.load() == nullptr &&
          hp->ptr.compare_exchange_strong(
              t_ptr, impl::HazardPointerRecord<T, RcuTraits>::kUsed)) {
        return hp;
      }

      hp = hp->next;
    }
    return nullptr;
  }

  impl::HazardPointerRecord<T, RcuTraits>& MakeHazardPointer() const {
    auto* hp = MakeHazardPointerCached();
    if (!hp) {
      hp = MakeHazardPointerFast();
      // all buckets are full, create a new one
      if (!hp) hp = MakeHazardPointerSlow();

      auto& cache = impl::cache<T, RcuTraits>;
      cache.hp = hp;
      cache.variable = this;
      cache.variable_epoch = epoch_;
    }
    UASSERT(&hp->owner == this);
    return *hp;
  }

  impl::HazardPointerRecord<T, RcuTraits>* MakeHazardPointerSlow() const {
    // allocate new pointer, and add it to the list (atomically)
    auto hp = new impl::HazardPointerRecord<T, RcuTraits>(*this);
    impl::HazardPointerRecord<T, RcuTraits>* old_hp = nullptr;
    do {
      old_hp = hp_record_head_.load();
      hp->next = old_hp;
    } while (!hp_record_head_.compare_exchange_strong(old_hp, hp));
    return hp;
  }

  void Retire(std::unique_ptr<T> old_ptr, std::unique_lock<MutexType>& lock) {
    LOG_TRACE() << "Retiring ptr=" << old_ptr.get();
    auto hazard_ptrs = CollectHazardPtrs(lock);

    if (hazard_ptrs.count(old_ptr.get()) > 0) {
      // old_ptr is being used now, we may not delete it, delay deletion
      LOG_TRACE() << "Not retire, still used ptr=" << old_ptr.get();
      retire_list_head_.push_back(std::move(old_ptr));
    } else {
      LOG_TRACE() << "Retire, not used ptr=" << old_ptr.get();
      DeleteAsync(std::move(old_ptr));
    }

    ScanRetiredList(hazard_ptrs);
  }

  // Scan retired list and for every object that has no more hazard_ptrs
  // pointing at it, destroy it (asynchronously)
  void ScanRetiredList(const std::unordered_set<T*>& hazard_ptrs) {
    for (auto rit = retire_list_head_.begin();
         rit != retire_list_head_.end();) {
      auto current = rit++;
      if (hazard_ptrs.count(current->get()) == 0) {
        // *current is not used by anyone, may delete it
        DeleteAsync(std::move(*current));
        retire_list_head_.erase(current);
      }
    }
  }

  // Returns all T*, that have hazard ptr pointing at them. Occasionally nullptr
  // might be in result as well.
  std::unordered_set<T*> CollectHazardPtrs(std::unique_lock<MutexType>&) {
    std::unordered_set<T*> hazard_ptrs;

    // Learn all currently used hazard pointers
    for (auto* hp = hp_record_head_.load(); hp; hp = hp->next) {
      hazard_ptrs.insert(hp->ptr.load());
    }
    return hazard_ptrs;
  }

  void DeleteAsync(std::unique_ptr<T> ptr) {
    switch (destruction_type_) {
      case DestructionType::kSync:
        ptr.reset();
        break;
      case DestructionType::kAsync:
        engine::CriticalAsyncNoSpan([ptr = std::move(ptr),
                                     token = wait_token_storage_
                                                 .GetToken()]() mutable {
          // Make sure *ptr is deleted before token is destroyed
          ptr.reset();
        }).Detach();
        break;
    }
  }

  const DestructionType destruction_type_;
  const uint64_t epoch_;

  mutable std::atomic<impl::HazardPointerRecord<T, RcuTraits>*> hp_record_head_{
      {nullptr}};

  MutexType mutex_;  // for current_ changes and retire_list_head_ access
  // may be read without mutex_ locked, but must be changed with held mutex_
  std::atomic<T*> current_;
  std::list<std::unique_ptr<T>> retire_list_head_;
  utils::impl::WaitTokenStorage wait_token_storage_;

  friend class ReadablePtr<T, RcuTraits>;
  friend class WritablePtr<T, RcuTraits>;
};

}  // namespace rcu

USERVER_NAMESPACE_END
