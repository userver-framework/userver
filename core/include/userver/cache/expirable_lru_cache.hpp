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
#include <userver/utils/statistics/writer.hpp>

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
impl::ExpirableValue<Value> Read(dump::Reader& reader, dump::To<impl::ExpirableValue<Value>>) {
    const auto [now, steady_now] = utils::impl::GetGlobalTime();
    // Evaluation order of arguments is guaranteed in brace-initialization.
    return impl::ExpirableValue<Value>{
        .value = reader.Read<Value>(),
        .update_time = reader.Read<std::chrono::system_clock::time_point>() - now + steady_now,
    };
}

}  // namespace impl

/// @ingroup userver_containers
/// @brief Class for expirable LRU cache.
///
/// Use @ref cache::LruMap for non-expirable LRU cache.
///
/// @snippet core/src/cache/expirable_lru_cache_test.cpp Sample ExpirableLruCache
template <typename Key, typename Value, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>>
class ExpirableLruCache final {
public:
    /// @brief Type of function used to compute or refresh a value for a key
    using UpdateValueFunc = std::function<Value(const Key&)>;

    /// @brief Cache read mode for @ref Get.
    enum class ReadMode {
        kSkipCache,  ///< Do not cache value got from update function
        kUseCache,   ///< Cache value got from update function
    };

    /// @brief Constructs the cache.
    ///
    /// @param ways Number of ways (shards). See @ref cache::NWayLRU.
    /// @param way_size Maximum size per way. See @ref cache::NWayLRU.
    /// @param hash Hash functor for keys.
    /// @param equal Equality functor for keys.
    ExpirableLruCache(size_t ways, size_t way_size, const Hash& hash = Hash(), const Equal& equal = Equal());

    /// @brief Destroys the cache and waits for all background update tasks to
    /// complete
    ~ExpirableLruCache();

    /// @brief Sets maximum size per way.
    ///
    /// @param way_size Maximum size per way. See @ref cache::NWayLRU.
    void SetWaySize(size_t way_size);

    /// @brief Returns the maximum lifetime of a cached value before it is
    /// considered expired.
    ///
    /// @return Current max lifetime, or zero if expiration is disabled.
    std::chrono::milliseconds GetMaxLifetime() const noexcept;

    /// @brief Sets the maximum lifetime of a cached value before it is
    /// considered expired.
    ///
    /// @param max_lifetime Max lifetime; zero disables expiration.
    void SetMaxLifetime(std::chrono::milliseconds max_lifetime);

    /// @brief Sets background update mode.
    ///
    /// If \p background_update is kDisabled, expiring values are not updated
    /// in background; if kEnabled, they are updated asynchronously when
    /// lifetime is near end.
    ///
    /// @param background_update Either kDisabled or kEnabled.
    void SetBackgroundUpdate(BackgroundUpdateMode background_update);

    /// @brief Returns value by key, or computes and optionally caches it.
    ///
    /// Equivalent to @ref GetOptional(\p key, \p update_func); if that is
    /// std::nullopt, returns update_func(\p key) and stores it in cache when
    /// \p read_mode is ReadMode::kUseCache.
    ///
    /// @param key Cache key.
    /// @param update_func Function to compute value when key is missing or
    /// expired.
    /// @param read_mode If kUseCache, caches the result of \p update_func.
    /// @return Cached or freshly computed value.
    Value Get(const Key& key, const UpdateValueFunc& update_func, ReadMode read_mode = ReadMode::kUseCache);

    /// @brief Returns value by key if present and not expired; may trigger
    /// background update.
    ///
    /// If background update is kEnabled and the entry is near expiry, schedules
    /// an async update via \p update_func. Does not block on that update.
    ///
    /// @param key Cache key.
    /// @param update_func Function used for background refresh when entry is
    /// near expiry.
    /// @return Value if key is in cache and not expired, otherwise std::nullopt.
    std::optional<Value> GetOptional(const Key& key, const UpdateValueFunc& update_func);

    /// @brief @ref GetOptional without expiry checks and without value updates.
    ///
    /// @param key Cache key.
    /// @return Value if key is in cache, otherwise std::nullopt.
    /// @note Used during fallback in FallbackELruCache.
    std::optional<Value> GetOptionalUnexpirable(const Key& key);

    /// @brief @ref GetOptional without expiry check; may trigger background update.
    ///
    /// @param key Cache key.
    /// @param update_func Function used for background refresh when entry is
    /// near expiry.
    /// @return Value if key is in cache, otherwise std::nullopt.
    /// @note Used during fallback in FallbackELruCache.
    std::optional<Value> GetOptionalUnexpirableWithUpdate(const Key& key, const UpdateValueFunc& update_func);

    /// @brief @ref GetOptional without triggering value updates (no background
    /// refresh).
    ///
    /// @param key Cache key.
    /// @return Value if key is in cache and not expired, otherwise std::nullopt.
    std::optional<Value> GetOptionalNoUpdate(const Key& key);

    /// @brief @ref GetOptionalNoUpdate, but returns the value together with its
    /// last update time.
    ///
    /// @param key Cache key.
    /// @return Value and update time if key is in cache and not expired,
    /// otherwise std::nullopt.
    std::optional<impl::ExpirableValue<Value>> GetOptionalNoUpdateWithLastUpdateTime(const Key& key);

    /// @brief Inserts or updates the value for the given key with current
    /// timestamp.
    ///
    /// @param key Cache key.
    /// @param value Value to store.
    void Put(const Key& key, const Value& value);

    /// @brief Inserts or updates the value for the given key with current
    /// timestamp (\p value is moved).
    ///
    /// @param key Cache key.
    /// @param value Value to store (moved).
    void Put(const Key& key, Value&& value);

    /// @brief Returns cache statistics (hits, misses, background updates, etc.).
    ///
    /// @return Reference to impl::ExpirableLruCacheStatistics.
    const impl::ExpirableLruCacheStatistics& GetStatistics() const;

    /// @brief Returns approximate number of entries in the cache.
    ///
    /// @return Approximate size (sum of all ways).
    size_t GetSizeApproximate() const;

    /// @brief Clears the cache (removes all entries).
    void Invalidate();

    /// @brief Erases the entry for the given key.
    ///
    /// @param key Cache key to erase.
    void InvalidateByKey(const Key& key);

    /// @brief Erases the key from cache if \p pred returns true for the
    /// current value.
    ///
    /// @param key Cache key.
    /// @param pred Predicate invoked with current value; entry is erased only
    /// if it returns true and value is not expired.
    template <typename Predicate>
    void InvalidateByKeyIf(const Key& key, Predicate pred);

    /// @brief Schedules an async task to update the value by update_func(\p key).
    ///
    /// @param key Cache key to update.
    /// @param update_func Function to compute the new value.
    void UpdateInBackground(const Key& key, UpdateValueFunc update_func);

    /// @brief Serializes cache state to the dump writer.
    ///
    /// Used for @ref dump::Dumper integration.
    ///
    /// @param writer Dump writer to write to.
    void Write(dump::Writer& writer) const;

    /// @brief Restores cache state from the dump reader.
    ///
    /// Used for @ref dump::Dumper integration.
    ///
    /// @param reader Dump reader to read from.
    void Read(dump::Reader& reader);

    /// @brief Sets the dumper that will be notified of cache updates.
    ///
    /// @param dumper Shared pointer to @ref dump::Dumper.
    /// @note This method is not thread-safe.
    void SetDumper(std::shared_ptr<dump::Dumper> dumper);

private:
    bool IsExpired(std::chrono::steady_clock::time_point update_time, std::chrono::steady_clock::time_point now) const;

    bool ShouldUpdate(std::chrono::steady_clock::time_point update_time, std::chrono::steady_clock::time_point now)
        const;

    cache::NWayLRU<Key, impl::ExpirableValue<Value>, Hash, Equal> lru_;
    std::atomic<std::chrono::milliseconds> max_lifetime_{std::chrono::milliseconds(0)};
    std::atomic<BackgroundUpdateMode> background_update_mode_{BackgroundUpdateMode::kDisabled};
    impl::ExpirableLruCacheStatistics stats_;
    concurrent::MutexSet<Key, Hash, Equal> mutex_set_;
    utils::impl::WaitTokenStorage wait_token_storage_;
};

template <typename Key, typename Value, typename Hash, typename Equal>
ExpirableLruCache<
    Key,
    Value,
    Hash,
    Equal>::ExpirableLruCache(size_t ways, size_t way_size, const Hash& hash, const Equal& equal)
    : lru_(ways, way_size, hash, equal),
      mutex_set_{ways, way_size, hash, equal}
{}

template <typename Key, typename Value, typename Hash, typename Equal>
ExpirableLruCache<Key, Value, Hash, Equal>::~ExpirableLruCache() {
    wait_token_storage_.WaitForAllTokens();
}

template <typename Key, typename Value, typename Hash, typename Equal>
void ExpirableLruCache<Key, Value, Hash, Equal>::SetWaySize(size_t way_size) {
    lru_.UpdateWaySize(way_size);
}

template <typename Key, typename Value, typename Hash, typename Equal>
std::chrono::milliseconds ExpirableLruCache<Key, Value, Hash, Equal>::GetMaxLifetime() const noexcept {
    return max_lifetime_.load();
}

template <typename Key, typename Value, typename Hash, typename Equal>
void ExpirableLruCache<Key, Value, Hash, Equal>::SetMaxLifetime(std::chrono::milliseconds max_lifetime) {
    max_lifetime_ = max_lifetime;
}

template <typename Key, typename Value, typename Hash, typename Equal>
void ExpirableLruCache<Key, Value, Hash, Equal>::SetBackgroundUpdate(BackgroundUpdateMode background_update) {
    background_update_mode_ = background_update;
}

template <typename Key, typename Value, typename Hash, typename Equal>
Value ExpirableLruCache<
    Key,
    Value,
    Hash,
    Equal>::Get(const Key& key, const UpdateValueFunc& update_func, ReadMode read_mode) {
    auto now = utils::datetime::SteadyNow();
    auto opt_old_value = GetOptional(key, update_func);
    if (opt_old_value) {
        return std::move(*opt_old_value);
    }

    auto mutex = mutex_set_.GetMutexForKey(key);
    const std::lock_guard lock(mutex);
    // Test one more time - concurrent ExpirableLruCache::Get()
    // might have put the value
    auto old_value = lru_.Get(key);
    if (old_value && !IsExpired(old_value->update_time, now)) {
        return std::move(old_value->value);
    }

    auto value = update_func(key);
    if (read_mode == ReadMode::kUseCache) {
        lru_.Put(key, {.value = value, .update_time = now});
    }
    return value;
}

template <typename Key, typename Value, typename Hash, typename Equal>
std::optional<Value> ExpirableLruCache<
    Key,
    Value,
    Hash,
    Equal>::GetOptional(const Key& key, const UpdateValueFunc& update_func) {
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
std::optional<Value> ExpirableLruCache<Key, Value, Hash, Equal>::GetOptionalUnexpirable(const Key& key) {
    auto old_value = lru_.Get(key);

    if (old_value) {
        impl::CacheHit(stats_);
        return old_value->value;
    }
    impl::CacheMiss(stats_);

    return std::nullopt;
}

template <typename Key, typename Value, typename Hash, typename Equal>
std::optional<Value> ExpirableLruCache<
    Key,
    Value,
    Hash,
    Equal>::GetOptionalUnexpirableWithUpdate(const Key& key, const UpdateValueFunc& update_func) {
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
std::optional<impl::ExpirableValue<Value>> ExpirableLruCache<
    Key,
    Value,
    Hash,
    Equal>::GetOptionalNoUpdateWithLastUpdateTime(const Key& key) {
    const auto now = utils::datetime::SteadyNow();
    const auto old_value = lru_.Get(key);
    if (old_value) {
        if (!IsExpired(old_value->update_time, now)) {
            impl::CacheHit(stats_);
            return old_value;
        } else {
            impl::CacheStale(stats_);
        }
    }
    impl::CacheMiss(stats_);
    return std::nullopt;
}

template <typename Key, typename Value, typename Hash, typename Equal>
std::optional<Value> ExpirableLruCache<Key, Value, Hash, Equal>::GetOptionalNoUpdate(const Key& key) {
    auto value_with_update_time = GetOptionalNoUpdateWithLastUpdateTime(key);
    if (value_with_update_time.has_value()) {
        return value_with_update_time->value;
    }
    return std::nullopt;
}

template <typename Key, typename Value, typename Hash, typename Equal>
void ExpirableLruCache<Key, Value, Hash, Equal>::Put(const Key& key, const Value& value) {
    lru_.Put(key, {.value = value, .update_time = utils::datetime::SteadyNow()});
}

template <typename Key, typename Value, typename Hash, typename Equal>
void ExpirableLruCache<Key, Value, Hash, Equal>::Put(const Key& key, Value&& value) {
    lru_.Put(key, {.value = std::move(value), .update_time = utils::datetime::SteadyNow()});
}

template <typename Key, typename Value, typename Hash, typename Equal>
const impl::ExpirableLruCacheStatistics& ExpirableLruCache<Key, Value, Hash, Equal>::GetStatistics() const {
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
void ExpirableLruCache<Key, Value, Hash, Equal>::InvalidateByKey(const Key& key) {
    lru_.InvalidateByKey(key);
}

template <typename Key, typename Value, typename Hash, typename Equal>
template <typename Predicate>
void ExpirableLruCache<Key, Value, Hash, Equal>::InvalidateByKeyIf(const Key& key, Predicate pred) {
    auto now = utils::datetime::SteadyNow();

    auto mutex = mutex_set_.GetMutexForKey(key);
    const std::lock_guard lock(mutex);
    const auto cur_value = lru_.Get(key);

    if (cur_value.has_value() && !IsExpired(cur_value->update_time, now) && pred(cur_value->value)) {
        InvalidateByKey(key);
    }
}

template <typename Key, typename Value, typename Hash, typename Equal>
void ExpirableLruCache<Key, Value, Hash, Equal>::UpdateInBackground(const Key& key, UpdateValueFunc update_func) {
    impl::CacheBackgroundUpdate(stats_);

    // cache will wait for all detached tasks in ~ExpirableLruCache()
    engine::DetachUnscopedUnsafe(engine::AsyncNoTracing(
        [token = wait_token_storage_.GetToken(), this, key, update_func = std::move(update_func)] {
            auto mutex = mutex_set_.GetMutexForKey(key);
            const std::unique_lock lock(mutex, std::try_to_lock);
            if (!lock) {
                // someone is updating the key right now
                return;
            }

            auto now = utils::datetime::SteadyNow();
            auto value = update_func(key);
            lru_.Put(key, {.value = value, .update_time = now});
        }
    ));
}

template <typename Key, typename Value, typename Hash, typename Equal>
bool ExpirableLruCache<
    Key,
    Value,
    Hash,
    Equal>::IsExpired(std::chrono::steady_clock::time_point update_time, std::chrono::steady_clock::time_point now)
    const {
    auto max_lifetime = max_lifetime_.load();
    return max_lifetime.count() != 0 && update_time + max_lifetime < now;
}

template <typename Key, typename Value, typename Hash, typename Equal>
bool ExpirableLruCache<
    Key,
    Value,
    Hash,
    Equal>::ShouldUpdate(std::chrono::steady_clock::time_point update_time, std::chrono::steady_clock::time_point now)
    const {
    auto max_lifetime = max_lifetime_.load();
    return (background_update_mode_.load() == BackgroundUpdateMode::kEnabled) && max_lifetime.count() != 0 &&
           update_time + max_lifetime / 2 < now;
}

/// @brief Wrapper around @ref ExpirableLruCache that binds an update function
/// for convenience.
///
/// @snippet core/src/cache/expirable_lru_cache_test.cpp Sample LruCacheWrapper
template <typename Key, typename Value, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>>
class LruCacheWrapper final {
public:
    /// @brief The underlying cache type
    using Cache = ExpirableLruCache<Key, Value, Hash, Equal>;
    /// @brief Read mode for @ref Get (same as Cache::ReadMode)
    using ReadMode = typename Cache::ReadMode;

    /// @brief Constructs wrapper with shared cache and update function.
    ///
    /// The same \p update_func is used for all @ref Get and @ref GetOptional
    /// calls.
    ///
    /// @param cache Shared pointer to @ref ExpirableLruCache.
    /// @param update_func Function to compute value when key is missing or
    /// expired.
    LruCacheWrapper(std::shared_ptr<Cache> cache, typename Cache::UpdateValueFunc update_func)
        : cache_(std::move(cache)),
          update_func_(std::move(update_func))
    {}

    /// @brief Returns cached value or computes it if key is missing in cache.
    ///
    /// @param key Cache key.
    /// @param read_mode If kUseCache, caches the computed value.
    /// @return Cached or freshly computed value.
    Value Get(const Key& key, ReadMode read_mode = ReadMode::kUseCache) {
        return cache_->Get(key, update_func_, read_mode);
    }

    /// @brief Returns cached value or std::nullopt if key is missing in cache.
    ///
    /// @param key Cache key.
    /// @return Value if key is in cache and not expired, otherwise std::nullopt.
    std::optional<Value> GetOptional(const Key& key) { return cache_->GetOptional(key, update_func_); }

    /// @brief Erases key from cache.
    ///
    /// @param key Cache key to erase.
    void InvalidateByKey(const Key& key) { cache_->InvalidateByKey(key); }

    /// @brief Erases key from cache if \p pred returns true for the current
    /// value.
    ///
    /// @param key Cache key.
    /// @param pred Predicate invoked with current value.
    template <typename Predicate>
    void InvalidateByKeyIf(const Key& key, Predicate pred) {
        cache_->InvalidateByKeyIf(key, pred);
    }

    /// @brief Schedules background update of cached value for the key.
    ///
    /// @param key Cache key to update.
    void UpdateInBackground(const Key& key) { cache_->UpdateInBackground(key, update_func_); }

    /// @brief Returns raw cache pointer.
    ///
    /// @return Shared pointer to the underlying @ref ExpirableLruCache.
    /// @note For internal use.
    std::shared_ptr<Cache> GetCache() { return cache_; }

private:
    std::shared_ptr<Cache> cache_;
    typename Cache::UpdateValueFunc update_func_;
};

template <typename Key, typename Value, typename Hash, typename Equal>
void ExpirableLruCache<Key, Value, Hash, Equal>::Write(dump::Writer& writer) const {
    utils::impl::UpdateGlobalTime();
    lru_.Write(writer);
}

template <typename Key, typename Value, typename Hash, typename Equal>
void ExpirableLruCache<Key, Value, Hash, Equal>::Read(dump::Reader& reader) {
    utils::impl::UpdateGlobalTime();
    lru_.Read(reader);
}

template <typename Key, typename Value, typename Hash, typename Equal>
void ExpirableLruCache<Key, Value, Hash, Equal>::SetDumper(std::shared_ptr<dump::Dumper> dumper) {
    lru_.SetDumper(std::move(dumper));
}

template <typename Key, typename Value, typename Hash, typename Equal>
void DumpMetric(utils::statistics::Writer& writer, const ExpirableLruCache<Key, Value, Hash, Equal>& cache) {
    writer["current-documents-count"] = cache.GetSizeApproximate();
    writer = cache.GetStatistics();
}

}  // namespace cache

USERVER_NAMESPACE_END
