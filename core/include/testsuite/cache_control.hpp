#pragma once

/// @file testsuite/cache_control.hpp
/// @brief @copybrief testsuite::CacheControl

#include <functional>
#include <unordered_set>
#include <vector>

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
  bool IsPeriodicUpdateEnabled(const cache::Config& cache_config,
                               const std::string& cache_name) const;

  void InvalidateAllCaches(
      cache::UpdateType update_type,
      const std::unordered_set<std::string>& names_blocklist);

  void InvalidateCaches(cache::UpdateType update_type,
                        std::unordered_set<std::string> names);

  void WriteCacheDumps(std::unordered_set<std::string> cache_names);

  void ReadCacheDumps(std::unordered_set<std::string> cache_names);

 private:
  friend class CacheInvalidatorHolder;

  void RegisterCache(cache::CacheUpdateTrait& cache);

  void UnregisterCache(cache::CacheUpdateTrait& cache);

  void ForEachCache(std::unordered_set<std::string> cache_names,
                    std::function<void(cache::CacheUpdateTrait&)> callback);

  const PeriodicUpdatesMode periodic_updates_mode_;

  engine::Mutex mutex_;
  std::vector<std::reference_wrapper<cache::CacheUpdateTrait>> caches_;
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
