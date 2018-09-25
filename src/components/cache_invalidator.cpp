#include <components/cache_invalidator.hpp>

namespace components {

CacheInvalidator::CacheInvalidator(const components::ComponentConfig&,
                                   const components::ComponentContext&) {}

void CacheInvalidator::InvalidateCaches() {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& invalidator : cache_invalidators_) {
    invalidator.handler();
  }
}

void CacheInvalidator::RegisterCacheInvalidator(
    components::CacheUpdateTrait& owner, std::function<void()>&& handler) {
  std::lock_guard<std::mutex> lock(mutex_);
  cache_invalidators_.emplace_back(&owner, std::move(handler));
}

void CacheInvalidator::UnregisterCacheInvalidator(
    components::CacheUpdateTrait& owner) {
  std::lock_guard<std::mutex> lock(mutex_);

  for (auto it = cache_invalidators_.begin(); it != cache_invalidators_.end();
       ++it) {
    if (it->owner == &owner) {
      std::swap(*it, cache_invalidators_.back());
      cache_invalidators_.pop_back();
      return;
    }
  }
}

}  // namespace components
