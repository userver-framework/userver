#include <cache/cache_invalidator.hpp>

namespace components {

namespace {
const std::string kCacheInvalidateSpanTag = "cache_invalidate";
}  // namespace

CacheInvalidator::CacheInvalidator(const components::ComponentConfig&,
                                   const components::ComponentContext&) {}

void CacheInvalidator::InvalidateCaches() {
  std::lock_guard<engine::Mutex> lock(mutex_);

  for (auto& invalidator : cache_invalidators_) {
    tracing::Span span(kCacheInvalidateSpanTag);
    invalidator.handler();
  }

  for (auto& invalidator : invalidators_) {
    tracing::Span span(kCacheInvalidateSpanTag);
    invalidator.handler();
  }
}

void CacheInvalidator::RegisterCacheInvalidator(
    components::CacheUpdateTrait& owner, Callback&& handler) {
  std::lock_guard<engine::Mutex> lock(mutex_);
  cache_invalidators_.emplace_back(&owner, std::move(handler));
}

void CacheInvalidator::UnregisterCacheInvalidator(
    components::CacheUpdateTrait& owner) {
  UnregisterInvalidatorGeneric(owner, cache_invalidators_);
}

void CacheInvalidator::RegisterComponentInvalidator(
    components::ComponentBase& owner, Callback&& handler) {
  std::lock_guard<engine::Mutex> lock(mutex_);
  invalidators_.emplace_back(&owner, std::move(handler));
}

void CacheInvalidator::UnregisterComponentInvalidator(
    components::ComponentBase& owner) {
  UnregisterInvalidatorGeneric(owner, invalidators_);
}

template <typename T>
void CacheInvalidator::UnregisterInvalidatorGeneric(
    T& owner, std::vector<Invalidator<T>>& invalidators) {
  std::lock_guard<engine::Mutex> lock(mutex_);

  for (auto it = invalidators.begin(); it != invalidators.end(); ++it) {
    if (it->owner == &owner) {
      std::swap(*it, invalidators.back());
      invalidators.pop_back();
      return;
    }
  }
}

}  // namespace components
