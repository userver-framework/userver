#pragma once

/// @file rcu/rcu.hpp
/// @brief Implementation of hazard pointer

#include <atomic>
#include <cstdlib>
#include <list>
#include <set>

#include <engine/async.hpp>
#include <engine/mutex.hpp>
#include <logging/log.hpp>
#include <utils/assert.hpp>
#include <utils/clang_format_workarounds.hpp>
#include <utils/impl/wait_token_storage.hpp>

/// @brief Read-Copy-Update
///
/// @see Based on ideas from
/// http://www.drdobbs.com/lock-free-data-structures-with-hazard-po/184401890
/// with modified API
namespace rcu {

template <typename T>
class Variable;

template <typename T>
class SharedReadablePtr;

namespace impl {

// Hazard pointer implementation. Pointers form a linked list. \p ptr points
// to the data they 'hold', next - to the next element in a list.
// kUsed is a filler value to show that hazard pointer is not free. Please see
// comment to it. Please note, pointers do not form a linked list with
// application-wide list, that is there is no 'static
// std::atomic<HazardPointerRecord*> global_head' Every rcu::Variable has it's
// own list of hazard pointers. Thus, move-assignment on hazard pointers is
// difficult to implement.
template <typename T>
struct HazardPointerRecord {
  // You see, objects are created 'filled', that is for the purposes of hazard
  // pointer list, they contain value. This eliminates some race conditions,
  // because these algorithms checks for ptr != nullptr (And kUsed is not
  // nullptr). Obviously, this value can't be used, when dereferencing it points
  // somewhere into kernel space and will cause SEGFAULT
  static T* const kUsed;

  // Pointer to data
  std::atomic<T*> ptr = kUsed;
  // Pointer to next element in linked list
  std::atomic<HazardPointerRecord*> next{{nullptr}};

  // Simple operation that marks this hazard pointer as no longer used.
  void Release() { ptr = nullptr; }
};

template <typename T>
T* const HazardPointerRecord<T>::kUsed = reinterpret_cast<T*>(1);

// Essentialy equal to variable.load(), but with better performance
// in hot path.
template <typename T>
T* ReadAtomicRMW(std::atomic<T*>& variable) {
  T* value = nullptr;
  variable.compare_exchange_strong(value, nullptr);
  return value;
}

template <typename T>
struct CachedData {
  impl::HazardPointerRecord<T>* hp{nullptr};
  const Variable<T>* variable{nullptr};
  // ensures that `variable` points to the instance that filled the cache
  uint64_t variable_epoch{0};
};

template <typename T>
// NOLINTNEXTLINE(misc-definitions-in-headers)
thread_local CachedData<T> cache;

uint64_t GetNextEpoch() noexcept;

}  // namespace impl

/// Reader smart pointer for rcu::Variable<T>. You may use operator*() or
/// operator->() to do something with the stored value. Once created,
/// ReadablePtr references the same immutable value: if Variable's value is
/// changed during ReadablePtr lifetime, it will not affect value referenced by
/// ReadablePtr.
template <typename T>
class USERVER_NODISCARD ReadablePtr final {
 public:
  explicit ReadablePtr(const Variable<T>& ptr)
      : hp_record_(&ptr.MakeHazardPointer()) {
    // This cycle guarantees that at the end of it both t_ptr_ and
    // hp_record_->ptr will both be set to
    // 1. something meaningful
    // 2. and that this meaningful value was not removed between assigning to
    //    t_ptr_ and storing  it in a hazard pointer
    do {
      t_ptr_ = ptr.GetCurrent();

      // std::memory_order_relaxed is OK here, there are 3 readers:
      // 1) MakeHazardPointer() doesn't care whether it reads old kUsed or new
      // t_ptr_ 2) Retire() will read new t_ptr_ value as it does RMW
      hp_record_->ptr.store(t_ptr_, std::memory_order_relaxed);
    } while (t_ptr_ != ptr.GetCurrent());
  }

  ReadablePtr(ReadablePtr<T>&& other) noexcept
      : t_ptr_(other.t_ptr_), hp_record_(other.hp_record_) {
    other.t_ptr_ = nullptr;
  }

  ReadablePtr& operator=(ReadablePtr<T>&& other) noexcept {
    // What do we have here?
    // 1. other may point to the same variable - or to a different one.
    // 2. therefore it's hazard pointer may be part of the same list,
    //    or of the different one.

    // We can't move hazard pointers between different lists - there is simply
    // no way to do this.
    // We also can't simply swap two ptrs inside hazard pointers and leave it be
    // - because if 'other' is from different Variable, then our ScanRetiredList
    // won't find any more pointer to current T* and will destroy object, while
    // 'other' is still holding a hazard pointer to it.
    // The thing is, whole ReadablePtr is just a glofiried holder for
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
    // freed. Just take values from other.
    hp_record_ = other.hp_record_;
    t_ptr_ = other.t_ptr_;

    // Now, it won't do us any good if there were two glofiried things having
    // pointer to same hp_record_. Kill the other one.
    other.t_ptr_ = nullptr;
    // We don't need to clean other.hp_record_, because other.t_ptr_ acts
    // like a guard to it. As long as other.t_ptr_ is nullptr, nobody will
    // use other.hp_record_

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
  // For SharedReadablePtr
  ReadablePtr(const rcu::Variable<T>& ptr, const rcu::ReadablePtr<T>& other)
      : t_ptr_(other.t_ptr_), hp_record_(&ptr.MakeHazardPointer()) {
    hp_record_->ptr.store(t_ptr_);
  }

  const T* GetOnRvalue() {
    static_assert(!sizeof(T),
                  "Don't use temporary ReadablePtr, store it to a variable");
    std::abort();
  }

  friend class SharedReadablePtr<T>;

  // This is a pointer to actual data. If it is null, then we treat it as
  // an indicator that this ReadablePtr is cleared and won't call
  // any logic accosiated with hp_record_
  T* t_ptr_;
  // Our hazard pointer. It can be nullptr in some circumstances.
  // Invariant is this: if t_ptr_ is not nullptr, then hp_record_ is also
  // not nullptr and points to hazard pointer containing same T*.
  // Thus it follows, that if t_ptr_ is nullptr, then hp_record_ is undefined.
  impl::HazardPointerRecord<T>* hp_record_;
};

/// Same as `rcu::ReadablePtr<T>`, but copyable
template <typename T>
class SharedReadablePtr final {
 public:
  explicit SharedReadablePtr(const rcu::Variable<T>& variable)
      : base_(variable), variable_(&variable) {}

  SharedReadablePtr(const SharedReadablePtr& other)
      : base_(*other.variable_, other.base_), variable_(other.variable_) {}

  SharedReadablePtr& operator=(const SharedReadablePtr& other) {
    *this = SharedReadablePtr{other};
    return *this;
  }

  SharedReadablePtr(SharedReadablePtr&& other) noexcept = default;
  SharedReadablePtr& operator=(SharedReadablePtr&& other) noexcept = default;

  const T* Get() const& { return base_.Get(); }
  const T* Get() && { return GetOnRvalue(); }

  const T* operator->() const& { return Get(); }
  const T* operator->() && { return GetOnRvalue(); }

  const T& operator*() const& { return *Get(); }
  const T& operator*() && { return *GetOnRvalue(); }

 private:
  const T* GetOnRvalue() {
    static_assert(
        !sizeof(T),
        "Don't use temporary SharedReadablePtr, store it to a variable");
    std::abort();
  }

  ReadablePtr<T> base_;
  const Variable<T>* variable_;
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
  // We cannot use the next version for current state duplication because
  // multiple writers might be waiting on `var.mutex_`.
  explicit WritablePtr(Variable<T>& var)
      : var_(var),
        lock_(var.mutex_),
        ptr_(std::make_unique<T>(*var_.GetCurrent())) {
    LOG_TRACE() << "Start writing ptr=" << ptr_.get();
  }

  WritablePtr(Variable<T>& var, T initial_value)
      : var_(var),
        lock_(var.mutex_),
        ptr_(std::make_unique<T>(std::move(initial_value))) {
    LOG_TRACE() << "Start writing ptr=" << ptr_.get()
                << " with custom initial value";
  }

  WritablePtr(Variable<T>& var, std::unique_ptr<T> initial_value)
      : var_(var), lock_(var.mutex_), ptr_(std::move(initial_value)) {
    UASSERT(ptr_ != nullptr);
    LOG_TRACE() << "Start writing ptr=" << ptr_.get()
                << " with custom initial value";
  }

  WritablePtr(WritablePtr<T>&& other) noexcept
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

  Variable<T>& var_;
  std::unique_lock<engine::Mutex> lock_;
  std::unique_ptr<T> ptr_;
};

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
/// ## Example usage:
///
/// @snippet rcu/rcu_test.cpp  Sample rcu::Variable usage
///
/// @see @ref md_en_userver_synchronization
template <typename T>
class Variable final {
 public:
  explicit Variable(std::unique_ptr<T> ptr)
      : epoch_(impl::GetNextEpoch()), current_(ptr.release()) {
    UASSERT(current_);
  }

  template <typename... Args>
  Variable(Args&&... args)
      : epoch_(impl::GetNextEpoch()),
        current_(new T(std::forward<Args>(args)...)) {}

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
    wait_token_storage_.WaitForAllTokens();
  }

  /// Obtain a smart pointer which can be used to read the current value.
  ReadablePtr<T> Read() const { return ReadablePtr<T>(*this); }

  /// Same as `Read`, but the pointer is copyable.
  SharedReadablePtr<T> ReadShared() const {
    return SharedReadablePtr<T>(*this);
  }

  /// Obtain a copy of contained value.
  T ReadCopy() const {
    auto ptr = Read();
    return *ptr;
  }

  /// Obtain a smart pointer which can be used to make changes to the
  /// current value and to set the Variable to the changed value.
  /// @note it will not compile if `T` has no copy constructor,
  ///       use Assign() instead.
  WritablePtr<T> StartWrite() { return WritablePtr<T>(*this); }

  /// Replaces Variable value with the provided one
  /// @note it doesn't copy `T`, so it works for types without
  ///       copy constructor
  void Assign(T new_value) {
    WritablePtr<T>(*this, std::move(new_value)).Commit();
  }

  // Replaces Variable value with the value, provided by unique_ptr
  void AssignPtr(std::unique_ptr<T> new_value) {
    WritablePtr<T>(*this, std::move(new_value)).Commit();
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

  impl::HazardPointerRecord<T>* MakeHazardPointerCached() const {
    auto& cache = impl::cache<T>;
    auto* hp = cache.hp;
    T* ptr = nullptr;
    if (hp && cache.variable == this && cache.variable_epoch == epoch_) {
      if (hp->ptr.load() == nullptr &&
          hp->ptr.compare_exchange_strong(
              ptr, impl::HazardPointerRecord<T>::kUsed)) {
        return hp;
      }
    }

    return nullptr;
  }

  impl::HazardPointerRecord<T>* MakeHazardPointerFast() const {
    // Look for any hazard pointer with nullptr data ptr.
    // Mark it with kUsed (to reserve it for ourselves) and return it.
    auto* hp = hp_record_head_.load();
    while (hp) {
      T* t_ptr = nullptr;
      if (hp->ptr.load() == nullptr &&
          hp->ptr.compare_exchange_strong(
              t_ptr, impl::HazardPointerRecord<T>::kUsed)) {
        return hp;
      }

      hp = hp->next;
    }
    return nullptr;
  }

  impl::HazardPointerRecord<T>& MakeHazardPointer() const {
    auto* hp = MakeHazardPointerCached();
    if (hp) return *hp;

    hp = MakeHazardPointerFast();
    // all buckets are full, create a new one
    if (!hp) hp = MakeHazardPointerSlow();

    auto& cache = impl::cache<T>;
    cache.hp = hp;
    cache.variable = this;
    cache.variable_epoch = epoch_;
    return *hp;
  }

  impl::HazardPointerRecord<T>* MakeHazardPointerSlow() const {
    // allocate new pointer, and add it to the list (atomically)
    auto hp = new impl::HazardPointerRecord<T>();
    impl::HazardPointerRecord<T>* old_hp;
    do {
      old_hp = hp_record_head_.load();
      hp->next = old_hp;
    } while (!hp_record_head_.compare_exchange_strong(old_hp, hp));
    return hp;
  }

  void Retire(std::unique_ptr<T> old_ptr,
              std::unique_lock<engine::Mutex>& lock) {
    LOG_TRACE() << "Reting ptr=" << old_ptr.get();
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
  void ScanRetiredList(const std::set<T*>& hazard_ptrs) {
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

  // Returns all T*, that have hazard ptr pointing at them. Ocasionally nullptr
  // might be in result as well.
  std::set<T*> CollectHazardPtrs(std::unique_lock<engine::Mutex>&) {
    std::set<T*> hazard_ptrs;

    // Learn all currently used hazard pointers
    for (auto* hp = hp_record_head_.load(); hp; hp = hp->next) {
      hazard_ptrs.insert(impl::ReadAtomicRMW(hp->ptr));
    }
    return hazard_ptrs;
  }

  void DeleteAsync(std::unique_ptr<T> ptr) {
    // Kill garbage asynchronously as T::~T() might be very slow
    engine::impl::CriticalAsync([ptr = std::move(ptr),
                                 token =
                                     wait_token_storage_.GetToken()]() mutable {
      // Make sure *ptr is deleted before token is destroyed
      ptr.reset();
    }).Detach();
  }

 private:
  const uint64_t epoch_;

  mutable std::atomic<impl::HazardPointerRecord<T>*> hp_record_head_{{nullptr}};

  engine::Mutex mutex_;  // for current_ changes and retire_list_head_ access
  // may be read without mutex_ locked, but must be changed with held mutex_
  std::atomic<T*> current_;
  std::list<std::unique_ptr<T>> retire_list_head_;
  utils::impl::WaitTokenStorage wait_token_storage_;

  friend class ReadablePtr<T>;
  friend class WritablePtr<T>;
};

}  // namespace rcu
