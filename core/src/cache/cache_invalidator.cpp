#include <cache/cache_invalidator.hpp>
#include <utils/periodic_task.hpp>

namespace components {

namespace {
const std::string kCacheInvalidateSpanTag = "cache_invalidate";
}  // namespace

CacheInvalidator::CacheInvalidator(const components::ComponentConfig&,
                                   const components::ComponentContext&) {}

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
  cache_invalidators_.emplace_back(&owner, std::move(handler));
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
  std::lock_guard<engine::Mutex> lock(mutex_);
  auto it = periodic_tasks_.find(name);
  if (it == periodic_tasks_.end()) {
    std::string error_msg = "Periodic task name " + name + " is not known";
    UASSERT_MSG(false, error_msg);
    throw std::runtime_error(error_msg);
  }
  return it->second.SynchronizeDebug(true);
}

}  // namespace components
