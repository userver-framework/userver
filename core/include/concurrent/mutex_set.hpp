#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include <engine/condition_variable.hpp>
#include <engine/mutex.hpp>
#include <engine/task/cancel.hpp>

namespace concurrent {

namespace impl {

template <typename T, typename Hash, typename Equal>
struct MutexDatum {
  explicit MutexDatum(size_t way_size, const Hash& hash = Hash{},
                      const Equal& equal = Equal{})
      : set{way_size, hash, equal} {}
  ~MutexDatum() {
    UASSERT_MSG(set.empty(),
                "MutexDatum is destroyed while someone is holding the lock");
  }

  engine::Mutex mutex;
  engine::ConditionVariable cv;
  std::unordered_set<T, Hash, Equal> set;
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
template <typename Key, typename Hash, typename Equal>
class ItemMutex final {
 public:
  ItemMutex(impl::MutexDatum<Key, Hash, Equal>& md, Key key);

  void lock();

  void unlock();

  bool try_lock();

 private:
  impl::MutexDatum<Key, Hash, Equal>& md_;
  const Key key_;
};

/// A dynamic set of mutexes. It can be used for separate critical sections for
/// multiple keys when the key set is not known at compile time and may change
/// in runtime.
template <typename Key = std::string, typename Hash = std::hash<Key>,
          typename Equal = std::equal_to<Key>>
class MutexSet final {
 public:
  explicit MutexSet(size_t ways = 1, size_t way_size = 1,
                    const Hash& hash = Hash{}, const Equal& equal = Equal{});

  /// Get the mutex-like object for a key. Coroutine-safe.
  /// @note the returned object holds a reference to MutexSet, so make sure
  ///       that MutexSet is alive while you're working with ItemMutex.
  ItemMutex<Key, Hash, Equal> GetMutexForKey(const Key& key);

 private:
  std::vector<std::unique_ptr<impl::MutexDatum<Key, Hash, Equal>>> mutex_data_;
};

template <typename Key, typename Hash, typename Equal>
MutexSet<Key, Hash, Equal>::MutexSet(size_t ways, size_t way_size,
                                     const Hash& hash, const Equal& equal) {
  mutex_data_.reserve(ways);
  for (size_t i = 0; i < ways; ++i) {
    mutex_data_.emplace_back(
        std::make_unique<impl::MutexDatum<Key, Hash, Equal>>(way_size, hash,
                                                             equal));
  }
}

template <typename Key, typename Hash, typename Equal>
ItemMutex<Key, Hash, Equal> MutexSet<Key, Hash, Equal>::GetMutexForKey(
    const Key& key) {
  auto& md = *mutex_data_[Hash()(key) % mutex_data_.size()];
  return ItemMutex<Key, Hash, Equal>(md, key);
}

template <typename Key, typename Hash, typename Equal>
ItemMutex<Key, Hash, Equal>::ItemMutex(impl::MutexDatum<Key, Hash, Equal>& md,
                                       Key key)
    : md_(md), key_(std::move(key)) {}

template <typename Key, typename Hash, typename Equal>
void ItemMutex<Key, Hash, Equal>::lock() {
  engine::TaskCancellationBlocker blocker;
  std::unique_lock<engine::Mutex> lock(md_.mutex);

  [[maybe_unused]] auto is_unlocked = md_.cv.Wait(lock, [this] {
    auto [it, inserted] = md_.set.insert(key_);
    return inserted;
  });
  UASSERT(is_unlocked);
}

template <typename Key, typename Hash, typename Equal>
void ItemMutex<Key, Hash, Equal>::unlock() {
  std::unique_lock<engine::Mutex> lock(md_.mutex);
  [[maybe_unused]] auto count = md_.set.erase(key_);
  UASSERT_MSG(count == 1, "unlocking not locked mutex");

  md_.cv.NotifyOne();
}

template <typename Key, typename Hash, typename Equal>
bool ItemMutex<Key, Hash, Equal>::try_lock() {
  std::unique_lock<engine::Mutex> lock(md_.mutex);
  auto [it, inserted] = md_.set.insert(key_);
  return inserted;
}

}  // namespace concurrent
