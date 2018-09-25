#pragma once

#include <components/cache_update_trait.hpp>
#include <components/component_context.hpp>
#include <server/handlers/tests_control.hpp>

namespace server {

class CacheInvalidatorHolder {
 public:
  CacheInvalidatorHolder(components::CacheUpdateTrait& cache,
                         const components::ComponentContext&);

  ~CacheInvalidatorHolder();

 private:
  server::handlers::TestsControl* tests_control_;
  components::CacheUpdateTrait& cache_;
};

}  // namespace server
