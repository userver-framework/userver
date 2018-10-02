#include <components/cache_invalidator.hpp>

namespace components {

namespace {
const std::string kCacheInvalidateSpanTag = "cache_invalidate";
}  // namespace

CacheInvalidator::CacheInvalidator(const components::ComponentConfig&,
                                   const components::ComponentContext&) {}

void CacheInvalidator::InvalidateCaches(tracing::Span& span) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& invalidator : cache_invalidators_) {
    invalidator.handler(span.CreateChild(kCacheInvalidateSpanTag));
  }
}

void CacheInvalidator::RegisterCacheInvalidator(
    components::CacheUpdateTrait& owner, Callback&& handler) {
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
