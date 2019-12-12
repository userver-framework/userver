#include <cache/testsuite_support.hpp>
#include <utils/periodic_task.hpp>

namespace components {

namespace {
const std::string kCacheInvalidateSpanTag = "cache_invalidate";
const std::string kPeriodicUpdateEnabled = "testsuite-periodic-update-enabled";
const std::string kForcePeriodicUpdate = "testsuite-force-periodic-update";
}  // namespace

TestsuiteSupport::TestsuiteSupport(const components::ComponentConfig& config,
                                   const components::ComponentContext&)
    : periodic_update_enabled_(
          config.ParseOptionalBool(kPeriodicUpdateEnabled)) {}

void TestsuiteSupport::InvalidateCaches(cache::UpdateType update_type,
                                        const std::vector<std::string>& names) {
  std::lock_guard<engine::Mutex> lock(mutex_);

  for (auto& invalidator : cache_invalidators_) {
    if (!DoesInvalidatorMatch(invalidator, names)) continue;
    tracing::Span span(kCacheInvalidateSpanTag);
    invalidator.handler(update_type);
  }

  for (auto& invalidator : invalidators_) {
    tracing::Span span(kCacheInvalidateSpanTag);
    invalidator.handler();
  }
}

void TestsuiteSupport::RegisterCacheInvalidator(
    components::CacheUpdateTrait& owner, CallbackUpdateType&& handler) {
  std::lock_guard<engine::Mutex> lock(mutex_);
  cache_invalidators_.emplace_back(&owner, std::move(handler));
}

void TestsuiteSupport::UnregisterCacheInvalidator(
    components::CacheUpdateTrait& owner) {
  UnregisterInvalidatorGeneric(owner, cache_invalidators_);
}

void TestsuiteSupport::RegisterComponentInvalidator(
    components::impl::ComponentBase& owner, CallbackVoid&& handler) {
  std::lock_guard<engine::Mutex> lock(mutex_);
  invalidators_.emplace_back(&owner, std::move(handler));
}

void TestsuiteSupport::UnregisterComponentInvalidator(
    components::impl::ComponentBase& owner) {
  UnregisterInvalidatorGeneric(owner, invalidators_);
}

template <typename T, typename Callback>
void TestsuiteSupport::UnregisterInvalidatorGeneric(
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

void TestsuiteSupport::RegisterPeriodicTask(const std::string& name,
                                            utils::PeriodicTask& task) {
  std::lock_guard<engine::Mutex> lock(mutex_);

  if (periodic_tasks_.find(name) != periodic_tasks_.end()) {
    std::string error_msg = "Periodic task name " + name + " already registred";
    UASSERT_MSG(false, error_msg);
    throw std::runtime_error(error_msg);
  }
  periodic_tasks_.emplace(name, task);
}

void TestsuiteSupport::UnregisterPeriodicTask(const std::string& name,
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

bool TestsuiteSupport::RunPeriodicTask(const std::string& name) {
  auto& task = FindPeriodicTask(name);
  std::lock_guard<engine::Mutex> lock(periodic_task_mutex_);
  return task.SynchronizeDebug(true);
}

utils::PeriodicTask& TestsuiteSupport::FindPeriodicTask(
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

bool TestsuiteSupport::IsPeriodicUpdateEnabled(
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

bool TestsuiteSupport::DoesInvalidatorMatch(
    const CacheInvalidatorStruct& invalidator,
    const std::vector<std::string>& names) const {
  if (names.empty()) {
    return true;
  }
  return std::find(names.begin(), names.end(), invalidator.owner->GetName()) !=
         names.end();
}

}  // namespace components
