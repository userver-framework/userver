#include <server/cache_invalidator_holder.hpp>

namespace server {

CacheInvalidatorHolder::CacheInvalidatorHolder(
    components::UpdatingComponentBase& cache,
    const components::ComponentContext& context)
    : cache_(cache) {
  tests_control_ = context.FindComponent<handlers::TestsControl>();
  assert(tests_control_ != nullptr);
  tests_control_->RegisterCacheInvalidator(
      cache_,
      std::bind(&components::UpdatingComponentBase::UpdateFull, &cache));
}

CacheInvalidatorHolder::~CacheInvalidatorHolder() {
  tests_control_->UnregisterCacheInvalidator(cache_);
}

}  // namespace server
