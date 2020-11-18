#pragma once

/// @file testsuite/cache_control.hpp
/// @brief @copybrief testsuite::CacheControl

#include <functional>
#include <list>

#include <cache/cache_update_trait.hpp>
#include <components/component_config.hpp>
#include <engine/mutex.hpp>

namespace testsuite {

/// @brief Periodically updated caches control interface for testsuite
/// @details All methods are coro-safe.
class CacheControl final {
 public:
  enum class PeriodicUpdatesMode { kDefault, kEnabled, kDisabled };

  explicit CacheControl(PeriodicUpdatesMode);

  /// Whether the cache with specified config should be updated periodically
  bool IsPeriodicUpdateEnabled(const cache::CacheConfigStatic& cache_config,
                               const std::string& cache_name) const;

  void InvalidateAllCaches(cache::UpdateType update_type);

  void InvalidateCaches(cache::UpdateType update_type,
                        const std::vector<std::string>& names);

 private:
  friend class CacheInvalidatorHolder;

  using Callback =
      std::function<void(cache::CacheUpdateTrait&, cache::UpdateType)>;

  struct Invalidator {
    Invalidator(cache::CacheUpdateTrait&, Callback);

    cache::CacheUpdateTrait* owner;
    Callback callback;
  };

  void RegisterCacheInvalidator(cache::CacheUpdateTrait& owner,
                                Callback callback);

  void UnregisterCacheInvalidator(cache::CacheUpdateTrait& owner);

  const PeriodicUpdatesMode periodic_updates_mode_;

  engine::Mutex mutex_;
  std::list<Invalidator> invalidators_;
};

/// RAII helper for testsuite registration
class CacheInvalidatorHolder final {
 public:
  CacheInvalidatorHolder(CacheControl&, cache::CacheUpdateTrait&);
  ~CacheInvalidatorHolder();

  CacheInvalidatorHolder(const CacheInvalidatorHolder&) = delete;
  CacheInvalidatorHolder(CacheInvalidatorHolder&&) = delete;
  CacheInvalidatorHolder& operator=(const CacheInvalidatorHolder&) = delete;
  CacheInvalidatorHolder& operator=(CacheInvalidatorHolder&&) = delete;

 private:
  CacheControl& cache_control_;
  cache::CacheUpdateTrait& cache_;
};

}  // namespace testsuite
