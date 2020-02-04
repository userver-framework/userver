#pragma once

#include <cache/cache_config.hpp>
#include <cache/nway_lru_cache.hpp>
#include <concurrent/mutex_set.hpp>
#include <engine/async.hpp>
#include <logging/log.hpp>
#include <utils/datetime.hpp>
#include <utils/impl/wait_token_storage.hpp>
#include <utils/statistics/recentperiod.hpp>

namespace cache {

struct ExpirableLruCacheStatisticsBase {
  std::atomic<size_t> hits{0};
  std::atomic<size_t> misses{0};
  std::atomic<size_t> stale{0};
  std::atomic<size_t> background_updates{0};

  ExpirableLruCacheStatisticsBase() = default;

  ExpirableLruCacheStatisticsBase(const ExpirableLruCacheStatisticsBase& other)
      : hits(other.hits.load()),
        misses(other.misses.load()),
        stale(other.stale.load()),
        background_updates(other.background_updates.load()) {}

  void Reset() {
    hits = 0;
    misses = 0;
    stale = 0;
    background_updates = 0;
  }

  ExpirableLruCacheStatisticsBase& operator+=(
      const ExpirableLruCacheStatisticsBase& other) {
    hits += other.hits.load();
    misses += other.misses.load();
    stale += other.stale.load();
    background_updates += other.background_updates.load();
    return *this;
  }
};

struct ExpirableLruCacheStatistics {
  ExpirableLruCacheStatisticsBase total;
  utils::statistics::RecentPeriod<ExpirableLruCacheStatisticsBase,
                                  ExpirableLruCacheStatisticsBase>
      recent{std::chrono::seconds(5), std::chrono::seconds(60)};
};

template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename Equal = std::equal_to<Key>>
class ExpirableLruCache final {
 public:
  using UpdateValueFunc = std::function<Value(const Key&)>;

  ExpirableLruCache(size_t ways, size_t way_size, const Hash& hash = Hash(),
                    const Equal& equal = Equal());

  ~ExpirableLruCache();

  void SetWaySize(size_t way_size);

  void SetMaxLifetime(std::chrono::milliseconds max_lifetime);

  void SetBackgroundUpdate(BackgroundUpdateMode background_update);

  Value Get(const Key& key, const UpdateValueFunc& update_func);

  const ExpirableLruCacheStatistics& GetStatistics() const;

  size_t GetSizeApproximate() const;

  void Invalidate();

  void InvalidateByKey(const Key& key);

 private:
  bool IsExpired(std::chrono::steady_clock::time_point update_time,
                 std::chrono::steady_clock::time_point now) const;

  bool ShouldUpdate(std::chrono::steady_clock::time_point update_time,
                    std::chrono::steady_clock::time_point now) const;

  void DoBackgroundUpdate(const Key& key, UpdateValueFunc update_func);

 private:
  struct MapValue {
    Value value;
    std::chrono::steady_clock::time_point update_time;
  };

  cache::NWayLRU<Key, MapValue, Hash, Equal> lru_;
  std::atomic<std::chrono::milliseconds> max_lifetime_{
      std::chrono::milliseconds(0)};
  std::atomic<BackgroundUpdateMode> background_update_mode_{
      BackgroundUpdateMode::kDisabled};
  ExpirableLruCacheStatistics stats_;
  concurrent::MutexSet<Key, Hash, Equal> mutex_set_;
  utils::impl::WaitTokenStorage wait_token_storage_;
};

template <typename Key, typename Value, typename Hash, typename Equal>
ExpirableLruCache<Key, Value, Hash, Equal>::ExpirableLruCache(
    size_t ways, size_t way_size, const Hash& hash, const Equal& equal)
    : lru_(ways, way_size, hash, equal),
      mutex_set_{ways, way_size, hash, equal} {}

template <typename Key, typename Value, typename Hash, typename Equal>
ExpirableLruCache<Key, Value, Hash, Equal>::~ExpirableLruCache() {
  wait_token_storage_.WaitForAllTokens();
}

template <typename Key, typename Value, typename Hash, typename Equal>
void ExpirableLruCache<Key, Value, Hash, Equal>::SetWaySize(size_t way_size) {
  lru_.UpdateWaySize(way_size);
}

template <typename Key, typename Value, typename Hash, typename Equal>
void ExpirableLruCache<Key, Value, Hash, Equal>::SetMaxLifetime(
    std::chrono::milliseconds max_lifetime) {
  max_lifetime_ = max_lifetime;
}

template <typename Key, typename Value, typename Hash, typename Equal>
void ExpirableLruCache<Key, Value, Hash, Equal>::SetBackgroundUpdate(
    BackgroundUpdateMode background_update) {
  background_update_mode_ = background_update;
}

template <typename Key, typename Value, typename Hash, typename Equal>
Value ExpirableLruCache<Key, Value, Hash, Equal>::Get(
    const Key& key, const UpdateValueFunc& update_func) {
  auto now = utils::datetime::SteadyNow();
  auto old_value = lru_.Get(key);

  if (old_value) {
    if (!IsExpired(old_value->update_time, now)) {
      stats_.total.hits++;
      stats_.recent.GetCurrentCounter().hits++;
      LOG_DEBUG() << "cache hit";

      if (ShouldUpdate(old_value->update_time, now)) {
        DoBackgroundUpdate(key, update_func);
      }
      return old_value->value;
    } else {
      stats_.total.stale++;
      stats_.recent.GetCurrentCounter().stale++;
      LOG_DEBUG() << "stale cache";
    }
  }

  LOG_DEBUG() << "cache miss";
  stats_.total.misses++;
  stats_.recent.GetCurrentCounter().misses++;

  auto mutex = mutex_set_.GetMutexForKey(key);
  std::lock_guard lock(mutex);
  // Test one more time - concurrent ExpirableLruCache::Get()
  // might have put the value
  old_value = lru_.Get(key);
  if (old_value && !IsExpired(old_value->update_time, now))
    return old_value->value;

  auto value = update_func(key);
  lru_.Put(key, {value, now});
  return value;
}

template <typename Key, typename Value, typename Hash, typename Equal>
const ExpirableLruCacheStatistics&
ExpirableLruCache<Key, Value, Hash, Equal>::GetStatistics() const {
  return stats_;
}

template <typename Key, typename Value, typename Hash, typename Equal>
size_t ExpirableLruCache<Key, Value, Hash, Equal>::GetSizeApproximate() const {
  return lru_.GetSize();
}

template <typename Key, typename Value, typename Hash, typename Equal>
void ExpirableLruCache<Key, Value, Hash, Equal>::Invalidate() {
  lru_.Invalidate();
}

template <typename Key, typename Value, typename Hash, typename Equal>
void ExpirableLruCache<Key, Value, Hash, Equal>::InvalidateByKey(
    const Key& key) {
  lru_.InvalidateByKey(key);
}

template <typename Key, typename Value, typename Hash, typename Equal>
bool ExpirableLruCache<Key, Value, Hash, Equal>::IsExpired(
    std::chrono::steady_clock::time_point update_time,
    std::chrono::steady_clock::time_point now) const {
  auto max_lifetime = max_lifetime_.load();
  return max_lifetime.count() != 0 && update_time + max_lifetime < now;
}

template <typename Key, typename Value, typename Hash, typename Equal>
bool ExpirableLruCache<Key, Value, Hash, Equal>::ShouldUpdate(
    std::chrono::steady_clock::time_point update_time,
    std::chrono::steady_clock::time_point now) const {
  auto max_lifetime = max_lifetime_.load();
  return (background_update_mode_.load() == BackgroundUpdateMode::kEnabled) &&
         max_lifetime.count() != 0 && update_time + max_lifetime / 2 < now;
}

template <typename Key, typename Value, typename Hash, typename Equal>
void ExpirableLruCache<Key, Value, Hash, Equal>::DoBackgroundUpdate(
    const Key& key, UpdateValueFunc update_func) {
  stats_.total.background_updates++;
  stats_.recent.GetCurrentCounter().background_updates++;

  // cache will wait for all detached tasks in ~ExpirableLruCache()
  engine::impl::Async([token = wait_token_storage_.GetToken(), this, key,
                       update_func = std::move(update_func)] {
    auto mutex = mutex_set_.GetMutexForKey(key);
    std::unique_lock lock(mutex, std::try_to_lock);
    if (!lock) {
      // someone is updating the key right now
      return;
    }

    auto now = utils::datetime::SteadyNow();
    auto value = update_func(key);
    lru_.Put(key, {value, now});
  })
      .Detach();
}

template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename Equal = std::equal_to<Key>>
class LruCacheWrapper final {
 public:
  using Cache = ExpirableLruCache<Key, Value, Hash, Equal>;
  LruCacheWrapper(std::shared_ptr<Cache> cache,
                  typename Cache::UpdateValueFunc update_func)
      : cache_(std::move(cache)), update_func_(std::move(update_func)) {}

  /// Get cached value or evaluates if key is missing in cache
  Value Get(const Key& key) { return cache_->Get(key, update_func_); }

  void InvalidateByKey(const Key& key) { cache_->InvalidateByKey(key); }

  /// Get raw cache. For internal use.
  std::shared_ptr<Cache> GetCache() { return cache_; }

 private:
  std::shared_ptr<Cache> cache_;
  typename Cache::UpdateValueFunc update_func_;
};

}  // namespace cache
