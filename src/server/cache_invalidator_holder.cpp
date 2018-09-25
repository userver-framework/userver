#include <server/cache_invalidator_holder.hpp>

#include <components/cache_invalidator.hpp>

namespace server {

CacheInvalidatorHolder::CacheInvalidatorHolder(
    components::CacheUpdateTrait& cache,
    const components::ComponentContext& context)
    : cache_invalidator_{context.FindComponentRequired<
          components::CacheInvalidator>()},
      cache_(cache) {
  cache_invalidator_->RegisterCacheInvalidator(
      cache_, std::bind(&components::CacheUpdateTrait::UpdateFull, &cache));
}

CacheInvalidatorHolder::~CacheInvalidatorHolder() {
  cache_invalidator_->UnregisterCacheInvalidator(cache_);
}

}  // namespace server
