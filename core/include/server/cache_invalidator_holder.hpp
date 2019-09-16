#pragma once

#include <cache/cache_invalidator.hpp>
#include <cache/cache_update_trait.hpp>
#include <cache/update_type.hpp>
#include <components/component_context.hpp>

namespace components {
class CacheInvalidator;
}  // namespace components

namespace server {

class CacheInvalidatorHolder final {
 public:
  CacheInvalidatorHolder(components::CacheUpdateTrait& cache,
                         const components::ComponentContext&);

  ~CacheInvalidatorHolder();

 private:
  components::CacheInvalidator& cache_invalidator_;
  components::CacheUpdateTrait& cache_;
};

template <typename T>
class ComponentInvalidatorHolder final {
 public:
  ComponentInvalidatorHolder(T& component,
                             const components::ComponentContext& context,
                             void (T::*invalidate_method)())
      : cache_invalidator_{context
                               .FindComponent<components::CacheInvalidator>()},
        component_(component) {
    cache_invalidator_.RegisterComponentInvalidator(
        component_, [component = &component_, invalidate_method]() {
          (component->*invalidate_method)();
        });
  }

  ~ComponentInvalidatorHolder() {
    cache_invalidator_.UnregisterComponentInvalidator(component_);
  }

 private:
  components::CacheInvalidator& cache_invalidator_;
  T& component_;
};

}  // namespace server
