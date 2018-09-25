#include <server/cache_invalidator_holder.hpp>

namespace server {

CacheInvalidatorHolder::CacheInvalidatorHolder(
    components::CacheUpdateTrait& cache,
    const components::ComponentContext& context)
    : tests_control_{context.FindComponent<handlers::TestsControl>()},
      cache_(cache) {
  if (!tests_control_)
    throw std::runtime_error{
        "CacheInvalidatorHolder requires handlers::TestsControl"};
  tests_control_->RegisterCacheInvalidator(
      cache_, std::bind(&components::CacheUpdateTrait::UpdateFull, &cache));
}

CacheInvalidatorHolder::~CacheInvalidatorHolder() {
  tests_control_->UnregisterCacheInvalidator(cache_);
}

}  // namespace server
