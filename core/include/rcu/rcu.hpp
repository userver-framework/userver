#pragma once

/// @file rcu/rcu.hpp
/// @brief Implementation of hazard pointer

#include <atomic>
#include <list>
#include <set>

#include <engine/mutex.hpp>
#include <logging/log.hpp>
#include <utils/assert.hpp>

/// Read-Cope-Update variable
/// @see Based on ideas from
/// http://www.drdobbs.com/lock-free-data-structures-with-hazard-po/184401890
/// with modified API
namespace rcu {

template <typename T>
class Variable;

namespace impl {

template <typename T>
struct HazardPointerRecord {
  static T* const kUsed;

  std::atomic<T*> ptr = kUsed;
  std::atomic<HazardPointerRecord*> next{{nullptr}};

  void Release() { ptr = nullptr; }
};

template <typename T>
T* const HazardPointerRecord<T>::kUsed = reinterpret_cast<T*>(1);

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
};

template <typename T>
// NOLINTNEXTLINE(misc-definitions-in-headers)
thread_local CachedData<T> cache;

}  // namespace impl

/// Reader smart pointer for rcu::Variable<T>. You may use operator*() or
/// operator->() to do something with the stored value. Once created,
/// ReadablePtr references the same immutable value: if Variable's value is
/// changed during ReadablePtr lifetime, it will not affect value referenced by
/// ReadablePtr.
template <typename T>
class ReadablePtr {
 public:
  explicit ReadablePtr(const Variable<T>& ptr)
      : hp_record_(ptr.MakeHazardPointer()) {
    do {
      t_ptr_ = ptr.GetCurrent();

      // std::memory_order_relaxed is OK here, there are 3 readers:
      // 1) MakeHazardPointer() doesn't care whether it reads old kUsed or new
      // t_ptr_ 2) Retire() will read new t_ptr_ value as it does RMW
      hp_record_.ptr.store(t_ptr_, std::memory_order_relaxed);
    } while (t_ptr_ != ptr.GetCurrent());

    LOG_TRACE() << "Start reading ptr=" << t_ptr_;
  }

  ReadablePtr(ReadablePtr<T>&& other) noexcept
      : t_ptr_(other.t_ptr_), hp_record_(other.hp_record_) {
    other.t_ptr_ = nullptr;

    LOG_TRACE() << "Continue reading ptr=" << t_ptr_;
  }

  ~ReadablePtr() {
    if (!t_ptr_) return;

    LOG_TRACE() << "Stop reading ptr=" << t_ptr_;
    hp_record_.Release();
  }

  const T& operator*() const { return *t_ptr_; }

  const T* operator->() const { return t_ptr_; }

 private:
  T* t_ptr_;
  impl::HazardPointerRecord<T>& hp_record_;
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
class WritablePtr {
 public:
  explicit WritablePtr(Variable<T>& var)
      : var_(var),
        lock_(var.mutex_),
        ptr_(std::make_unique<T>(*var.GetCurrent())) {
    LOG_TRACE() << "Start writing ptr=" << ptr_.get();
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

  T& operator*() { return *ptr_; }

  T* operator->() { return ptr_.get(); }

 private:
  Variable<T>& var_;
  std::unique_lock<engine::Mutex> lock_;
  std::unique_ptr<T> ptr_;
};

/// A variable with MT-access pattern "very often reads, seldom writes". It is
/// specially optimized for reads. On read, one obtains a ReaderPtr<T> from it
/// and uses the obtained value as long as it wants to. On write, one obtains a
/// WritablePtr<T> with a copy of the last version of the value, makes some
/// changes to it, and commits the result to update current variable value (does
/// Read-Copy-Update). Old version of the value is not freed on update, it will
/// be eventually freed when a subsequent writer identifies that nobody works
/// with this version.
template <typename T>
class Variable {
 public:
  explicit Variable(std::unique_ptr<T> ptr) : current_(ptr.release()) {}

  template <typename... Args>
  Variable(Args&&... args) : current_(new T(std::forward<Args>(args)...)) {}

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
  }

  /// Obtain a smart pointer which can be used to read the current value.
  ReadablePtr<T> Read() const { return ReadablePtr<T>(*this); }

  /// Obtain a smart pointer which can be used to make changes to the
  /// current value and to set the Variable to the changed value.
  WritablePtr<T> StartWrite() { return WritablePtr<T>(*this); }

 private:
  T* GetCurrent() const { return current_.load(); }

  impl::HazardPointerRecord<T>* MakeHazardPointerCached() const {
    auto& cache = impl::cache<T>;
    auto* hp = cache.hp;
    T* ptr = nullptr;
    if (hp && cache.variable == this) {
      if (hp->ptr.load() == nullptr &&
          hp->ptr.compare_exchange_strong(
              ptr, impl::HazardPointerRecord<T>::kUsed)) {
        return hp;
      }
    }

    return nullptr;
  }

  impl::HazardPointerRecord<T>* MakeHazardPointerFast() const {
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
    cache.variable = this;
    cache.hp = hp;
    return *hp;
  }

  impl::HazardPointerRecord<T>* MakeHazardPointerSlow() const {
    auto hp = new impl::HazardPointerRecord<T>();
    impl::HazardPointerRecord<T>* old_hp;
    do {
      old_hp = hp_record_head_.load();
      hp->next = old_hp;
    } while (!hp_record_head_.compare_exchange_strong(old_hp, hp));
    return hp;
  }

  void Retire(std::unique_ptr<T> old_ptr, std::unique_lock<engine::Mutex>&) {
    LOG_TRACE() << "Reting ptr=" << old_ptr.get();
    std::set<T*> hazard_ptrs;

    // Learn all currently used hazard pointers
    for (auto* hp = hp_record_head_.load(); hp; hp = hp->next) {
      hazard_ptrs.insert(impl::ReadAtomicRMW(hp->ptr));
    }

    if (hazard_ptrs.count(old_ptr.get()) > 0) {
      // old_ptr is being used now, we may not delete it, delay deletion
      LOG_TRACE() << "Not retire, still used ptr=" << old_ptr.get();
      retire_list_head_.push_back(std::move(old_ptr));
    } else {
      LOG_TRACE() << "Retire, not used ptr=" << old_ptr.get();
    }

    ScanRetiredList(hazard_ptrs);
  }

  void ScanRetiredList(const std::set<T*>& hazard_ptrs) {
    for (auto rit = retire_list_head_.begin();
         rit != retire_list_head_.end();) {
      auto current = rit++;
      if (hazard_ptrs.count(current->get()) == 0) {
        // *current is not used by anyone, may delete it
        retire_list_head_.erase(current);
      }
    }
  }

 private:
  mutable std::atomic<impl::HazardPointerRecord<T>*> hp_record_head_{{nullptr}};

  engine::Mutex mutex_;  // for current_ changes and retire_list_head_ access
  // may be read without mutex_ locked, but must be changed with held mutex_
  std::atomic<T*> current_;
  std::list<std::unique_ptr<T>> retire_list_head_;

  friend class ReadablePtr<T>;
  friend class WritablePtr<T>;
};

}  // namespace rcu
