#include <cache/cache_invalidator.hpp>
#include <utils/periodic_task.hpp>

namespace components {

namespace {
const std::string kCacheInvalidateSpanTag = "cache_invalidate";
const std::string kPeriodicUpdateEnabled = "testsuite-periodic-update-enabled";
const std::string kForcePeriodicUpdate = "testsuite-force-periodic-update";

const std::string kConfigCacheName = "taxi-config-client-updater";
}  // namespace

CacheInvalidator::CacheInvalidator(const components::ComponentConfig& config,
                                   const components::ComponentContext&)
    : periodic_update_enabled_(
          config.ParseOptionalBool(kPeriodicUpdateEnabled)) {}

void CacheInvalidator::InvalidateCaches(cache::UpdateType update_type) {
  std::lock_guard<engine::Mutex> lock(mutex_);

  for (auto& invalidator : cache_invalidators_) {
    tracing::Span span(kCacheInvalidateSpanTag);
    invalidator.handler(update_type);
  }

  for (auto& invalidator : invalidators_) {
    tracing::Span span(kCacheInvalidateSpanTag);
    invalidator.handler();
  }
}

void CacheInvalidator::RegisterCacheInvalidator(
    components::CacheUpdateTrait& owner, CallbackUpdateType&& handler) {
  std::lock_guard<engine::Mutex> lock(mutex_);
  // Hack to ensure that config is invalidated first, see TAXIDATA-1543
  if (owner.GetName() == kConfigCacheName) {
    cache_invalidators_.insert(cache_invalidators_.begin(),
                               {&owner, std::move(handler)});
  } else {
    cache_invalidators_.emplace_back(&owner, std::move(handler));
  }
}

void CacheInvalidator::UnregisterCacheInvalidator(
    components::CacheUpdateTrait& owner) {
  UnregisterInvalidatorGeneric(owner, cache_invalidators_);
}

void CacheInvalidator::RegisterComponentInvalidator(
    components::impl::ComponentBase& owner, CallbackVoid&& handler) {
  std::lock_guard<engine::Mutex> lock(mutex_);
  invalidators_.emplace_back(&owner, std::move(handler));
}

void CacheInvalidator::UnregisterComponentInvalidator(
    components::impl::ComponentBase& owner) {
  UnregisterInvalidatorGeneric(owner, invalidators_);
}

template <typename T, typename Callback>
void CacheInvalidator::UnregisterInvalidatorGeneric(
    T& owner, std::vector<Invalidator<T, Callback>>& invalidators) {
  std::lock_guard<engine::Mutex> lock(mutex_);

  for (auto it = invalidators.begin(); it != invalidators.end(); ++it) {
    if (it->owner == &owner) {
      std::swap(*it, invalidators.back());
      invalidators.pop_back();
      return;
    }
  }
}

void CacheInvalidator::RegisterPeriodicTask(const std::string& name,
                                            utils::PeriodicTask& task) {
  std::lock_guard<engine::Mutex> lock(mutex_);

  if (periodic_tasks_.find(name) != periodic_tasks_.end()) {
    std::string error_msg = "Periodic task name " + name + " already registred";
    UASSERT_MSG(false, error_msg);
    throw std::runtime_error(error_msg);
  }
  periodic_tasks_.emplace(name, task);
}

void CacheInvalidator::UnregisterPeriodicTask(const std::string& name,
                                              utils::PeriodicTask& task) {
  std::lock_guard<engine::Mutex> lock(mutex_);
  auto it = periodic_tasks_.find(name);
  if (it == periodic_tasks_.end()) {
    UASSERT_MSG(false, "Periodic task name " + name + " is not known");
    LOG_ERROR() << "Periodic task name " << name << " is not known";
  } else {
    if (&it->second != &task) {
      UASSERT_MSG(false, "Periodic task name " + name +
                             "  does not belong to the caller");
      LOG_ERROR() << "Periodic task name " << name
                  << " does not belong to the caller";
    }

    periodic_tasks_.erase(name);
  }
}

bool CacheInvalidator::RunPeriodicTask(const std::string& name) {
  auto& task = FindPeriodicTask(name);
  std::lock_guard<engine::Mutex> lock(periodic_task_mutex_);
  return task.SynchronizeDebug(true);
}

utils::PeriodicTask& CacheInvalidator::FindPeriodicTask(
    const std::string& name) {
  std::lock_guard<engine::Mutex> lock(mutex_);
  auto it = periodic_tasks_.find(name);
  if (it == periodic_tasks_.end()) {
    std::string error_msg = "Periodic task name " + name + " is not known";
    UASSERT_MSG(false, error_msg);
    throw std::runtime_error(error_msg);
  }
  return it->second;
}

bool CacheInvalidator::IsPeriodicUpdateEnabled(
    const ComponentConfig& cache_component_config,
    const std::string& cache_component_name) const {
  const auto& force_periodic_update =
      cache_component_config.ParseOptionalBool(kForcePeriodicUpdate);
  bool enabled;
  std::string reason;
  if (force_periodic_update) {
    enabled = *force_periodic_update;
    reason = "config";
  } else if (periodic_update_enabled_) {
    enabled = *periodic_update_enabled_;
    reason = std::string(kName) + " config";
  } else {
    enabled = true;
    reason = "default";
  }

  auto state = enabled ? "enabled" : "disabled";
  LOG_DEBUG() << cache_component_name << " periodic update is " << state
              << " by " << reason;
  return enabled;
}

}  // namespace components
