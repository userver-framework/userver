#pragma once

#include <mutex>
#include <optional>
#include <shared_mutex>  // for shared_lock

#include <engine/mutex.hpp>

/// Locking stuff
namespace concurrent {

/// Proxy class for locked access to data protected with locking::SharedLock<T>
template <typename Lock, typename Data>
class LockedPtr final {
 public:
  using Mutex = typename Lock::mutex_type;

  LockedPtr(Mutex& mutex, Data& data) : lock_(mutex), data_(data) {}
  LockedPtr(Lock&& lock, Data& data) : lock_(std::move(lock)), data_(data) {}

  Data& operator*() & { return data_; }

  /// Don't use *tmp for temporary value, store it to variable.
  Data& operator*() && { return *GetOnRvalue(); }

  Data* operator->() & { return &data_; }

  /// Don't use tmp-> for temporary value, store it to variable.
  Data* operator->() && { return GetOnRvalue(); }

  Lock& GetLock() { return lock_; }

 private:
  const Data* GetOnRvalue() {
    static_assert(false && sizeof(Data),
                  "Don't use temporary LockedPtr, store it to a variable");
    std::abort();
  }

  Lock lock_;
  Data& data_;
};

/// Container for shared data protected with a mutex of any type
/// (mutex, shared mutex, etc.).
template <typename Data, typename Mutex = ::engine::Mutex>
class Variable final {
 public:
  template <typename... Arg>
  Variable(Arg&&... arg) : data_(std::forward<Arg>(arg)...) {}

  LockedPtr<std::unique_lock<Mutex>, Data> UniqueLock() {
    return {mutex_, data_};
  }

  LockedPtr<std::unique_lock<Mutex>, const Data> UniqueLock() const {
    return {mutex_, data_};
  }

  std::optional<LockedPtr<std::unique_lock<Mutex>, const Data>> UniqueLock(
      std::try_to_lock_t) const {
    std::unique_lock<Mutex> lock(mutex_, std::try_to_lock);
    if (lock) {
      return LockedPtr<std::unique_lock<Mutex>, const Data>{std::move(lock),
                                                            data_};
    } else {
      return std::nullopt;
    }
  }

  std::optional<LockedPtr<std::unique_lock<Mutex>, const Data>> UniqueLock(
      std::chrono::milliseconds try_duration) const {
    std::unique_lock<Mutex> lock(mutex_, try_duration);
    if (lock) {
      return LockedPtr<std::unique_lock<Mutex>, const Data>{std::move(lock),
                                                            data_};
    } else {
      return std::nullopt;
    }
  }

  LockedPtr<std::shared_lock<Mutex>, const Data> SharedLock() const {
    return {mutex_, data_};
  }

  LockedPtr<std::lock_guard<Mutex>, Data> Lock() { return {mutex_, data_}; }

  LockedPtr<std::lock_guard<Mutex>, const Data> Lock() const {
    return {mutex_, data_};
  }

  /// Get raw mutex. Use with caution. For simple use cases call Lock(),
  /// UniqueLock(), SharedLock() instead.
  Mutex& GetMutexUnsafe() const { return mutex_; }

  /// Get raw data. Use with extreme caution, only for cases where it is
  /// impossible to access data with safe methods (e.g. std::scoped_lock with
  /// multiple mutexes). For simple use cases call Lock(), UniqueLock(),
  /// SharedLock() instead.
  Data& GetDataUnsafe() { return data_; }

  const Data& GetDataUnsafe() const { return data_; }

 private:
  mutable Mutex mutex_;
  Data data_;
};

}  // namespace concurrent
