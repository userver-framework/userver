#pragma once

/// @file testsuite/testsuite_support.hpp
/// @brief @copybrief components::TestsuiteSupport

#include <unordered_map>
#include <vector>

#include <cache/cache_update_trait.hpp>
#include <cache/update_type.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <engine/mutex.hpp>
#include <testsuite/cache_control.hpp>
#include <testsuite/component_control.hpp>
#include <testsuite/periodic_task_control.hpp>
#include <testsuite/postgres_control.hpp>
#include <testsuite/redis_control.hpp>
#include <utils/periodic_task.hpp>

/// Testsuite integration
namespace testsuite {}

namespace components {

// clang-format off

/// @brief Testsuite support component
///
/// Provides additional functionality for testing, e.g. forced cache updates.
///
/// ## Configuration example:
///
/// ```
/// testsuite-support:
///   testsuite-periodic-update-enabled: true
/// ```
///
/// ## Available options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// testsuite-periodic-update-enabled | whether caches update periodically | true

// clang-format on

class TestsuiteSupport final : public components::impl::ComponentBase {
 public:
  static constexpr const char* kName = "testsuite-support";

  TestsuiteSupport(const components::ComponentConfig& component_config,
                   const components::ComponentContext& component_context);

  testsuite::CacheControl& GetCacheControl();
  testsuite::ComponentControl& GetComponentControl();
  testsuite::PeriodicTaskControl& GetPeriodicTaskControl();
  const testsuite::PostgresControl& GetPostgresControl();
  const testsuite::RedisControl& GetRedisControl();

  void InvalidateEverything(cache::UpdateType update_type);

  void InvalidateCaches(cache::UpdateType update_type,
                        const std::vector<std::string>& names);

 private:
  engine::Mutex invalidation_mutex_;

  testsuite::CacheControl cache_control_;
  testsuite::ComponentControl component_control_;
  testsuite::PeriodicTaskControl periodic_task_control_;
  testsuite::PostgresControl postgres_control_;
  testsuite::RedisControl redis_control_;
};

}  // namespace components
