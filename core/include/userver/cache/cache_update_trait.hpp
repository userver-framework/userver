#pragma once

/// @file userver/cache/cache_update_trait.hpp
/// @brief @copybrief cache::CacheUpdateTrait

#include <memory>
#include <string>

#include <userver/cache/cache_statistics.hpp>
#include <userver/cache/update_type.hpp>
#include <userver/components/component_fwd.hpp>
#include <userver/dump/fwd.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/flags.hpp>

USERVER_NAMESPACE_BEGIN

namespace rcu {
template <typename T>
class ReadablePtr;
}  // namespace rcu

namespace cache {

struct CacheDependencies;
struct Config;

/// @ingroup userver_base_classes
///
/// @brief Base class for periodically updated caches.
///
/// @note Don't use directly, inherit from components::CachingComponentBase
/// instead
class CacheUpdateTrait {
 public:
  CacheUpdateTrait(CacheUpdateTrait&&) = delete;
  CacheUpdateTrait& operator=(CacheUpdateTrait&&) = delete;

  /// @brief Forces a cache update of specified type
  /// @throws If `Update` throws
  void Update(UpdateType update_type);

  /// @return name of the component
  const std::string& Name() const;

 protected:
  /// @cond
  // For internal use only
  CacheUpdateTrait(const components::ComponentConfig& config,
                   const components::ComponentContext& context);

  // For internal use only
  explicit CacheUpdateTrait(CacheDependencies&& dependencies);

  virtual ~CacheUpdateTrait();
  /// @endcond

  /// Update types configured for the cache
  AllowedUpdateTypes GetAllowedUpdateTypes() const;

  /// Periodic update flags
  enum class Flag {
    kNone = 0,
    kNoFirstUpdate = 1 << 0,  ///< Disable initial update on start
  };

  /// Starts periodic updates
  void StartPeriodicUpdates(utils::Flags<Flag> flags = {});

  /// @brief Stops periodic updates
  /// @warning Should be called in destructor of derived class.
  void StopPeriodicUpdates();

  void AssertPeriodicUpdateStarted();

  /// Called in `CachingComponentBase::Set` during update to indicate
  /// that the cached data has been modified
  void OnCacheModified();

  /// @cond
  // For internal use only
  rcu::ReadablePtr<Config> GetConfig() const;

  /// Checks for the presence of the flag for pre-assign check
  bool HasPreAssignCheck() const;

  // For internal use only
  // TODO remove after TAXICOMMON-3959
  engine::TaskProcessor& GetCacheTaskProcessor() const;
  /// @endcond

  /// @brief Should be overridden in a derived class to implement the update
  /// logic. The statistics should be updated using `stats_scope`, and call
  /// `CachingComponentBase::Set` if the cached data has changed.
  ///
  /// @param type type of the update
  /// @param last_update time of the last update (value of `now` from previous
  /// invocation of Update or default constructed value if this is the first
  /// Update).
  /// @param now current time point
  ///
  /// @throws Any `std::exception` on error
  ///
  /// @see @ref md_en_userver_caches
  virtual void Update(UpdateType type,
                      const std::chrono::system_clock::time_point& last_update,
                      const std::chrono::system_clock::time_point& now,
                      UpdateStatisticsScope& stats_scope) = 0;

 private:
  virtual void Cleanup() = 0;

  virtual void MarkAsExpired();

  virtual void GetAndWrite(dump::Writer& writer) const;

  virtual void ReadAndSet(dump::Reader& reader);

  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace cache

USERVER_NAMESPACE_END
