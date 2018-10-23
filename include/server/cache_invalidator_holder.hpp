#pragma once

#include <components/cache_update_trait.hpp>
#include <components/component_context.hpp>

namespace components {
class CacheInvalidator;
}  // namespace components

namespace server {

class CacheInvalidatorHolder {
 public:
  CacheInvalidatorHolder(components::CacheUpdateTrait& cache,
                         const components::ComponentContext&);

  ~CacheInvalidatorHolder();

 private:
  components::CacheInvalidator& cache_invalidator_;
  components::CacheUpdateTrait& cache_;
};

}  // namespace server
