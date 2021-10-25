#pragma once

/// @file userver/concurrent/mutex_set.hpp
/// @brief @copybrief concurrent::MutexSet

#include <chrono>
#include <functional>
#include <string>
#include <unordered_set>
#include <utility>

#include <userver/engine/condition_variable.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/cached_hash.hpp>
#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

namespace impl {

template <typename T, typename Equal>
struct MutexDatum final {
  explicit MutexDatum(size_t way_size, const Equal& equal = Equal{})
      : set(way_size, {}, equal) {}

  ~MutexDatum() {
    UASSERT_MSG(set.empty(),
                "MutexDatum is destroyed while someone is holding the lock");
  }

  engine::Mutex mutex;
  engine::ConditionVariable cv;
  std::unordered_set<T, std::hash<T>, Equal> set;
};

}  // namespace impl

/// Mutex-like object associated with the key of a MutexSet. It provides the
/// same interface as engine::Mutex, you may use it with std::unique_lock,
/// std::lock_guard, etc.
/// @note different ItemMutex'es of the same MutexSet obtained with the same
///       argument to GetMutexForKey() share the same critical section. IOW, if
///       the first mutex is locked, the second one will block until the first
///       mutex is unlocked.
/// @note can be used only from coroutines.
template <typename Key, typename Equal>
class ItemMutex final {
 public:
  using HashAndKey = utils::CachedHash<Key>;

  using MutexDatum =
      impl::MutexDatum<HashAndKey, utils::CachedHashKeyEqual<Equal>>;

  ItemMutex(MutexDatum& md, HashAndKey&& key);

  void lock();

  void unlock();

  bool try_lock();

  template <typename Rep, typename Period>
  bool try_lock_for(std::chrono::duration<Rep, Period>);

  template <typename Clock, typename Duration>
  bool try_lock_until(std::chrono::time_point<Clock, Duration>);

 private:
  bool TryFinishLocking();

  MutexDatum& md_;
  const HashAndKey key_;
};

/// @ingroup userver_concurrency userver_containers
///
/// @brief A dynamic set of mutexes
///
/// It can be used for separate critical sections for
/// multiple keys when the key set is not known at compile time and may change
/// in runtime.
///
/// Example:
/// @snippet src/concurrent/mutex_set_test.cpp  Sample mutex set usage
template <typename Key = std::string, typename Hash = std::hash<Key>,
          typename Equal = std::equal_to<Key>>
class MutexSet final : Hash {
 public:
  explicit MutexSet(size_t ways = 1, size_t way_size = 1,
                    const Hash& hash = Hash{}, const Equal& equal = Equal{});

  /// Get the mutex-like object for a key. Coroutine-safe.
  /// @note the returned object holds a reference to MutexSet, so make sure
  ///       that MutexSet is alive while you're working with ItemMutex.
  ItemMutex<Key, Equal> GetMutexForKey(Key key);

 private:
  using MutexDatum = typename ItemMutex<Key, Equal>::MutexDatum;
  utils::FixedArray<MutexDatum> mutex_data_;
};

template <typename Key, typename Hash, typename Equal>
MutexSet<Key, Hash, Equal>::MutexSet(size_t ways, size_t way_size,
                                     const Hash& hash, const Equal& equal)
    : Hash(hash),
      mutex_data_(ways, way_size, utils::CachedHashKeyEqual<Equal>{equal}) {}

template <typename Key, typename Hash, typename Equal>
ItemMutex<Key, Equal> MutexSet<Key, Hash, Equal>::GetMutexForKey(Key key) {
  const auto hash_value = Hash::operator()(key);
  const auto size = mutex_data_.size();

  // Compilers optimize a % b and a / b to a single div operation
  const auto way = hash_value % size;
  const auto new_hash = hash_value / size;

  return ItemMutex<Key, Equal>(mutex_data_[way], {new_hash, std::move(key)});
}

template <typename Key, typename Equal>
ItemMutex<Key, Equal>::ItemMutex(MutexDatum& md, HashAndKey&& key)
    : md_(md), key_(std::move(key)) {}

template <typename Key, typename Equal>
void ItemMutex<Key, Equal>::lock() {
  engine::TaskCancellationBlocker blocker;
  std::unique_lock<engine::Mutex> lock(md_.mutex);

  [[maybe_unused]] auto is_locked =
      md_.cv.Wait(lock, [this] { return TryFinishLocking(); });
  UASSERT(is_locked);
}

template <typename Key, typename Equal>
void ItemMutex<Key, Equal>::unlock() {
  std::unique_lock lock(md_.mutex);

  // Forcing the destructor of the node to run outside of the critical section
  [[maybe_unused]] auto node = md_.set.extract(key_);

  // We must wakeup all the waiters, because otherwise we can wake up the wrong
  // one and loose the wakeup:
  //
  // Task #1 waits for mutex A, task #2 waits for mutex B
  // Task #3
  //  * unlocks mutex A
  //  * does NotifyOne
  //  * wakeups thread #2
  //  * mutex B is still locked, task #2 goes to sleep
  //  * we lost the wakeup :(
  md_.cv.NotifyAll();

  lock.unlock();

  // Destructor of node is run here
}

template <typename Key, typename Equal>
bool ItemMutex<Key, Equal>::try_lock() {
  std::unique_lock<engine::Mutex> lock(md_.mutex);
  return TryFinishLocking();
}

template <typename Key, typename Equal>
template <typename Rep, typename Period>
bool ItemMutex<Key, Equal>::try_lock_for(
    std::chrono::duration<Rep, Period> duration) {
  std::unique_lock<engine::Mutex> lock(md_.mutex);
  return md_.cv.WaitFor(lock, duration, [this] { return TryFinishLocking(); });
}

template <typename Key, typename Equal>
template <typename Clock, typename Duration>
bool ItemMutex<Key, Equal>::try_lock_until(
    std::chrono::time_point<Clock, Duration> time_point) {
  std::unique_lock<engine::Mutex> lock(md_.mutex);
  return md_.cv.WaitUntil(lock, time_point,
                          [this] { return TryFinishLocking(); });
}

template <typename Key, typename Equal>
bool ItemMutex<Key, Equal>::TryFinishLocking() {
  return md_.set.insert(key_).second;
}

}  // namespace concurrent

USERVER_NAMESPACE_END
