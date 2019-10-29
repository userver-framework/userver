#include <server/cache_invalidator_holder.hpp>

#include <cache/testsuite_support.hpp>

namespace server {

CacheInvalidatorHolder::CacheInvalidatorHolder(
    components::CacheUpdateTrait& cache,
    const components::ComponentContext& context)
    : testsuite_support_{context.FindComponent<components::TestsuiteSupport>()},
      cache_(cache) {
  testsuite_support_.RegisterCacheInvalidator(
      cache_, [cache = &cache_](cache::UpdateType update_type) {
        cache->Update(update_type);
      });
}

CacheInvalidatorHolder::~CacheInvalidatorHolder() {
  testsuite_support_.UnregisterCacheInvalidator(cache_);
}

}  // namespace server
