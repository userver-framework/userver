#pragma once

#include <cache/nway_lru_cache.hpp>
#include <logging/log.hpp>
#include <utils/datetime.hpp>
#include <utils/statistics/recentperiod.hpp>

namespace cache {

struct ExpirableLruCacheStatisticsBase {
  std::atomic<size_t> hits{0};
  std::atomic<size_t> misses{0};
  std::atomic<size_t> stale{0};

  ExpirableLruCacheStatisticsBase() = default;

  ExpirableLruCacheStatisticsBase(const ExpirableLruCacheStatisticsBase& other)
      : hits(other.hits.load()),
        misses(other.misses.load()),
        stale(other.stale.load()) {}

  void Reset() {
    hits = 0;
    misses = 0;
    stale = 0;
  }

  ExpirableLruCacheStatisticsBase& operator+=(
      const ExpirableLruCacheStatisticsBase& other) {
    hits += other.hits.load();
    misses += other.misses.load();
    stale += other.stale.load();
    return *this;
  }
};

struct ExpirableLruCacheStatistics {
  ExpirableLruCacheStatisticsBase total;
  utils::statistics::RecentPeriod<ExpirableLruCacheStatisticsBase,
                                  ExpirableLruCacheStatisticsBase>
      recent{std::chrono::seconds(5), std::chrono::seconds(60)};
};

template <typename Key, typename Value>
class ExpirableLruCache final {
 public:
  using UpdateValueFunc = std::function<Value(const Key&)>;

  ExpirableLruCache(size_t ways, size_t way_size);

  void UpdateWaySize(size_t way_size);

  void UpdateMaxLifetime(std::chrono::milliseconds max_lifetime);

  Value Get(const Key& key, const UpdateValueFunc& update_func);

  const ExpirableLruCacheStatistics& GetStatistics() const;

  size_t GetSizeApproximate() const;

  void Invalidate();

  void InvalidateByKey(const Key& key);

 private:
  bool IsExpired(std::chrono::steady_clock::time_point update_time,
                 std::chrono::steady_clock::time_point now) const;

 private:
  struct MapValue {
    Value value;
    std::chrono::steady_clock::time_point update_time;
  };

  cache::NWayLRU<Key, MapValue> lru_;
  std::atomic<std::chrono::milliseconds> max_lifetime_{
      std::chrono::milliseconds(0)};
  ExpirableLruCacheStatistics stats_;
};

template <typename Key, typename Value>
ExpirableLruCache<Key, Value>::ExpirableLruCache(size_t ways, size_t way_size)
    : lru_(ways, way_size) {}

template <typename Key, typename Value>
void ExpirableLruCache<Key, Value>::UpdateWaySize(size_t way_size) {
  lru_.UpdateWaySize(way_size);
}

template <typename Key, typename Value>
void ExpirableLruCache<Key, Value>::UpdateMaxLifetime(
    std::chrono::milliseconds max_lifetime) {
  max_lifetime_ = max_lifetime;
}

template <typename Key, typename Value>
Value ExpirableLruCache<Key, Value>::Get(const Key& key,
                                         const UpdateValueFunc& update_func) {
  auto now = utils::datetime::SteadyNow();
  auto old_value = lru_.Get(key, [this, now](const MapValue& value) {
    auto expired = IsExpired(value.update_time, now);
    if (expired) {
      stats_.total.stale++;
      stats_.recent.GetCurrentCounter().stale++;
    }
    return !expired;
  });

  if (old_value) {
    stats_.total.hits++;
    stats_.recent.GetCurrentCounter().hits++;
    LOG_DEBUG() << "cache hit";
    return old_value->value;
  }

  LOG_DEBUG() << "cache miss";
  stats_.total.misses++;
  stats_.recent.GetCurrentCounter().misses++;
  auto value = update_func(key);
  lru_.Put(key, {value, now});
  return value;
}

template <typename Key, typename Value>
const ExpirableLruCacheStatistics&
ExpirableLruCache<Key, Value>::GetStatistics() const {
  return stats_;
}

template <typename Key, typename Value>
size_t ExpirableLruCache<Key, Value>::GetSizeApproximate() const {
  return lru_.GetSize();
}

template <typename Key, typename Value>
void ExpirableLruCache<Key, Value>::Invalidate() {
  lru_.Invalidate();
}

template <typename Key, typename Value>
void ExpirableLruCache<Key, Value>::InvalidateByKey(const Key& key) {
  lru_.InvalidateByKey(key);
}

template <typename Key, typename Value>
bool ExpirableLruCache<Key, Value>::IsExpired(
    std::chrono::steady_clock::time_point update_time,
    std::chrono::steady_clock::time_point now) const {
  auto max_lifetime = max_lifetime_.load();
  return max_lifetime.count() != 0 && update_time + max_lifetime < now;
}

template <typename Key, typename Value>
class LruCacheWrapper final {
 public:
  using Cache = ExpirableLruCache<Key, Value>;
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
