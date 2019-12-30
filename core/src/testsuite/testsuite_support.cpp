#include <testsuite/testsuite_support.hpp>

#include <utils/periodic_task.hpp>

namespace components {

namespace {

const std::string kPeriodicUpdateEnabled = "testsuite-periodic-update-enabled";

testsuite::CacheControl::PeriodicUpdatesMode ParsePeriodicUpdatesMode(
    const boost::optional<bool>& config_value) {
  using PeriodicUpdatesMode = testsuite::CacheControl::PeriodicUpdatesMode;
  if (!config_value) return PeriodicUpdatesMode::kDefault;
  return *config_value ? PeriodicUpdatesMode::kEnabled
                       : PeriodicUpdatesMode::kDisabled;
}

}  // namespace

TestsuiteSupport::TestsuiteSupport(const components::ComponentConfig& config,
                                   const components::ComponentContext&)
    : cache_control_(ParsePeriodicUpdatesMode(
          config.ParseOptionalBool(kPeriodicUpdateEnabled))) {}

testsuite::CacheControl& TestsuiteSupport::GetCacheControl() {
  return cache_control_;
}

testsuite::ComponentControl& TestsuiteSupport::GetComponentControl() {
  return component_control_;
}

testsuite::PeriodicTaskControl& TestsuiteSupport::GetPeriodicTaskControl() {
  return periodic_task_control_;
}

void TestsuiteSupport::InvalidateEverything(cache::UpdateType update_type) {
  std::lock_guard lock(invalidation_mutex_);
  cache_control_.InvalidateAllCaches(update_type);
  component_control_.InvalidateComponents();
}

void TestsuiteSupport::InvalidateCaches(cache::UpdateType update_type,
                                        const std::vector<std::string>& names) {
  std::lock_guard lock(invalidation_mutex_);
  cache_control_.InvalidateCaches(update_type, names);
}

}  // namespace components
