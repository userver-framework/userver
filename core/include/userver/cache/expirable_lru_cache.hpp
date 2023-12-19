#pragma once

/// @file userver/cache/expirable_lru_cache.hpp
/// @brief @copybrief cache::ExpirableLruCache

#include <atomic>
#include <chrono>
#include <optional>

#include <userver/cache/lru_cache_config.hpp>
#include <userver/cache/lru_cache_statistics.hpp>
#include <userver/cache/nway_lru_cache.hpp>
#include <userver/concurrent/mutex_set.hpp>
#include <userver/dump/common.hpp>
#include <userver/dump/dumper.hpp>
#include <userver/engine/async.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/impl/cached_time.hpp>
#include <userver/utils/impl/wait_token_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

namespace impl {

template <typename Value>
struct ExpirableValue final {
  Value value;
  std::chrono::steady_clock::time_point update_time;
};

template <typename Value>
void Write(dump::Writer& writer, const impl::ExpirableValue<Value>& value) {
  const auto [now, steady_now] = utils::impl::GetGlobalTime();
  writer.Write(value.value);
  writer.Write(value.update_time - steady_now + now);
}

template <typename Value>
impl::ExpirableValue<Value> Read(dump::Reader& reader,
                                 dump::To<impl::ExpirableValue<Value>>) {
  const auto [now, steady_now] = utils::impl::GetGlobalTime();
  // Evaluation order of arguments is guaranteed in brace-initialization.
  return impl::ExpirableValue<Value>{
      reader.Read<Value>(),
      reader.Read<std::chrono::system_clock::time_point>() - now + steady_now};
}

}  // namespace impl

/// @ingroup userver_containers
/// @brief Class for expirable LRU cache. Use cache::LruMap for not expirable
/// LRU Cache.
///
/// Example usage:
///
/// @snippet cache/expirable_lru_cache_test.cpp Sample ExpirableLruCache
template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename Equal = std::equal_to<Key>>
class ExpirableLruCache final {
 public:
  using UpdateValueFunc = std::function<Value(const Key&)>;

  /// Cache read mode
  enum class ReadMode {
    kSkipCache,  ///< Do not cache value got from update function
    kUseCache,   ///< Cache value got from update function
  };

  ExpirableLruCache(size_t ways, size_t way_size, const Hash& hash = Hash(),
                    const Equal& equal = Equal());

  ~ExpirableLruCache();

  void SetWaySize(size_t way_size);

  std::chrono::milliseconds GetMaxLifetime() const noexcept;

  void SetMaxLifetime(std::chrono::milliseconds max_lifetime);

  /**
   * Sets background update mode. If "background_update" mode is kDisabled,
   * expiring values are not updated in background (asynchronously) or are
   * updated if "background_update" is kEnabled.
   */
  void SetBackgroundUpdate(BackgroundUpdateMode background_update);

  /**
   * @returns GetOptional("key", update_func) if it is not std::nullopt.
   * Otherwise the result of update_func(key) is returned, and additionally
   * stored in cache if "read_mode" is kUseCache.
   */
  Value Get(const Key& key, const UpdateValueFunc& update_func,
            ReadMode read_mode = ReadMode::kUseCache);

  /**
   * Update value in cache by "update_func" if background update mode is
   * kEnabled and "key" is in cache and not expired but its lifetime ends soon.
   * @returns value by key if key is in cache and not expired, or std::nullopt
   * otherwise
   */
  std::optional<Value> GetOptional(const Key& key,
                                   const UpdateValueFunc& update_func);

  /**
   * GetOptional, but without expiry checks and value updates.
   *
   * Used during fallback in FallbackELruCache.
   */
  std::optional<Value> GetOptionalUnexpirable(const Key& key);

  /**
   * GetOptional, but without expiry check.
   *
   * Used during fallback in FallbackELruCache.
   */
  std::optional<Value> GetOptionalUnexpirableWithUpdate(
      const Key& key, const UpdateValueFunc& update_func);

  /**
   * GetOptional, but without value updates.
   */
  std::optional<Value> GetOptionalNoUpdate(const Key& key);

  void Put(const Key& key, const Value& value);

  void Put(const Key& key, Value&& value);

  const impl::ExpirableLruCacheStatistics& GetStatistics() const;

  size_t GetSizeApproximate() const;

  /// Clear cache
  void Invalidate();

  /// Erase key from cache
  void InvalidateByKey(const Key& key);

  /// Add async task for updating value by update_func(key)
  void UpdateInBackground(const Key& key, UpdateValueFunc update_func);

  void Write(dump::Writer& writer) const;

  void Read(dump::Reader& reader);

  /// The dump::Dumper will be notified of any cache updates. This method is not
  /// thread-safe.
  void SetDumper(std::shared_ptr<dump::Dumper> dumper);

 private:
  bool IsExpired(std::chrono::steady_clock::time_point update_time,
                 std::chrono::steady_clock::time_point now) const;

  bool ShouldUpdate(std::chrono::steady_clock::time_point update_time,
                    std::chrono::steady_clock::time_point now) const;

  cache::NWayLRU<Key, impl::ExpirableValue<Value>, Hash, Equal> lru_;
  std::atomic<std::chrono::milliseconds> max_lifetime_{
      std::chrono::milliseconds(0)};
  std::atomic<BackgroundUpdateMode> background_update_mode_{
      BackgroundUpdateMode::kDisabled};
  impl::ExpirableLruCacheStatistics stats_;
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
std::chrono::milliseconds
ExpirableLruCache<Key, Value, Hash, Equal>::GetMaxLifetime() const noexcept {
  return max_lifetime_.load();
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
    const Key& key, const UpdateValueFunc& update_func, ReadMode read_mode) {
  auto now = utils::datetime::SteadyNow();
  auto opt_old_value = GetOptional(key, update_func);
  if (opt_old_value) {
    return std::move(*opt_old_value);
  }

  auto mutex = mutex_set_.GetMutexForKey(key);
  std::lock_guard lock(mutex);
  // Test one more time - concurrent ExpirableLruCache::Get()
  // might have put the value
  auto old_value = lru_.Get(key);
  if (old_value && !IsExpired(old_value->update_time, now)) {
    return std::move(old_value->value);
  }

  auto value = update_func(key);
  if (read_mode == ReadMode::kUseCache) {
    lru_.Put(key, {value, now});
  }
  return value;
}

template <typename Key, typename Value, typename Hash, typename Equal>
std::optional<Value> ExpirableLruCache<Key, Value, Hash, Equal>::GetOptional(
    const Key& key, const UpdateValueFunc& update_func) {
  auto now = utils::datetime::SteadyNow();
  auto old_value = lru_.Get(key);

  if (old_value) {
    if (!IsExpired(old_value->update_time, now)) {
      impl::CacheHit(stats_);

      if (ShouldUpdate(old_value->update_time, now)) {
        UpdateInBackground(key, update_func);
      }

      return std::move(old_value->value);
    } else {
      impl::CacheStale(stats_);
    }
  }
  impl::CacheMiss(stats_);

  return std::nullopt;
}

template <typename Key, typename Value, typename Hash, typename Equal>
std::optional<Value>
ExpirableLruCache<Key, Value, Hash, Equal>::GetOptionalUnexpirable(
    const Key& key) {
  auto old_value = lru_.Get(key);

  if (old_value) {
    impl::CacheHit(stats_);
    return old_value->value;
  }
  impl::CacheMiss(stats_);

  return std::nullopt;
}

template <typename Key, typename Value, typename Hash, typename Equal>
std::optional<Value>
ExpirableLruCache<Key, Value, Hash, Equal>::GetOptionalUnexpirableWithUpdate(
    const Key& key, const UpdateValueFunc& update_func) {
  auto now = utils::datetime::SteadyNow();
  auto old_value = lru_.Get(key);

  if (old_value) {
    impl::CacheHit(stats_);

    if (ShouldUpdate(old_value->update_time, now)) {
      UpdateInBackground(key, update_func);
    }

    return old_value->value;
  }
  impl::CacheMiss(stats_);

  return std::nullopt;
}

template <typename Key, typename Value, typename Hash, typename Equal>
std::optional<Value>
ExpirableLruCache<Key, Value, Hash, Equal>::GetOptionalNoUpdate(
    const Key& key) {
  auto now = utils::datetime::SteadyNow();
  auto old_value = lru_.Get(key);

  if (old_value) {
    if (!IsExpired(old_value->update_time, now)) {
      impl::CacheHit(stats_);

      return old_value->value;
    } else {
      impl::CacheStale(stats_);
    }
  }
  impl::CacheMiss(stats_);

  return std::nullopt;
}

template <typename Key, typename Value, typename Hash, typename Equal>
void ExpirableLruCache<Key, Value, Hash, Equal>::Put(const Key& key,
                                                     const Value& value) {
  lru_.Put(key, {value, utils::datetime::SteadyNow()});
}

template <typename Key, typename Value, typename Hash, typename Equal>
void ExpirableLruCache<Key, Value, Hash, Equal>::Put(const Key& key,
                                                     Value&& value) {
  lru_.Put(key, {std::move(value), utils::datetime::SteadyNow()});
}

template <typename Key, typename Value, typename Hash, typename Equal>
const impl::ExpirableLruCacheStatistics&
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
void ExpirableLruCache<Key, Value, Hash, Equal>::UpdateInBackground(
    const Key& key, UpdateValueFunc update_func) {
  stats_.total.background_updates++;
  stats_.recent.GetCurrentCounter().background_updates++;

  // cache will wait for all detached tasks in ~ExpirableLruCache()
  engine::AsyncNoSpan([token = wait_token_storage_.GetToken(), this, key,
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
  }).Detach();
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

template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename Equal = std::equal_to<Key>>
class LruCacheWrapper final {
 public:
  using Cache = ExpirableLruCache<Key, Value, Hash, Equal>;
  using ReadMode = typename Cache::ReadMode;

  LruCacheWrapper(std::shared_ptr<Cache> cache,
                  typename Cache::UpdateValueFunc update_func)
      : cache_(std::move(cache)), update_func_(std::move(update_func)) {}

  /// Get cached value or evaluates if "key" is missing in cache
  Value Get(const Key& key, ReadMode read_mode = ReadMode::kUseCache) {
    return cache_->Get(key, update_func_, read_mode);
  }

  /// Get cached value or "nullopt" if "key" is missing in cache
  std::optional<Value> GetOptional(const Key& key) {
    return cache_->GetOptional(key, update_func_);
  }

  void InvalidateByKey(const Key& key) { cache_->InvalidateByKey(key); }

  /// Update cached value in background
  void UpdateInBackground(const Key& key) {
    cache_->UpdateInBackground(key, update_func_);
  }

  /// Get raw cache. For internal use.
  std::shared_ptr<Cache> GetCache() { return cache_; }

 private:
  std::shared_ptr<Cache> cache_;
  typename Cache::UpdateValueFunc update_func_;
};

template <typename Key, typename Value, typename Hash, typename Equal>
void ExpirableLruCache<Key, Value, Hash, Equal>::Write(
    dump::Writer& writer) const {
  utils::impl::UpdateGlobalTime();
  lru_.Write(writer);
}

template <typename Key, typename Value, typename Hash, typename Equal>
void ExpirableLruCache<Key, Value, Hash, Equal>::Read(dump::Reader& reader) {
  utils::impl::UpdateGlobalTime();
  lru_.Read(reader);
}

template <typename Key, typename Value, typename Hash, typename Equal>
void ExpirableLruCache<Key, Value, Hash, Equal>::SetDumper(
    std::shared_ptr<dump::Dumper> dumper) {
  lru_.SetDumper(std::move(dumper));
}

template <typename Key, typename Value, typename Hash, typename Equal>
void DumpMetric(utils::statistics::Writer& writer,
                const ExpirableLruCache<Key, Value, Hash, Equal>& cache) {
  writer["current-documents-count"] = cache.GetSizeApproximate();
  writer = cache.GetStatistics();
}

}  // namespace cache

USERVER_NAMESPACE_END
