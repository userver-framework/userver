#include <cache/cache_invalidator.hpp>

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

}  // namespace components
