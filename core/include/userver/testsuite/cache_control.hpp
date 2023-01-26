#pragma once

/// @file userver/testsuite/cache_control.hpp
/// @brief @copybrief testsuite::CacheControl

#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

#include <userver/cache/update_type.hpp>
#include <userver/components/component_fwd.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/utils/not_null.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {
class CacheUpdateTrait;
struct Config;
}  // namespace cache

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

  void InvalidateAllCaches(cache::UpdateType update_type);

  void InvalidateCaches(cache::UpdateType update_type,
                        std::unordered_set<std::string> names);

 private:
  friend class CacheInvalidatorHolder;

  void RegisterCache(cache::CacheUpdateTrait& cache);

  void UnregisterCache(cache::CacheUpdateTrait& cache);

  const PeriodicUpdatesMode periodic_updates_mode_;

  engine::Mutex mutex_;
  std::vector<utils::NotNull<cache::CacheUpdateTrait*>> caches_;
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

USERVER_NAMESPACE_END
