#pragma once

#include <components/component_context.hpp>
#include <components/updating_component_base.hpp>
#include <server/handlers/tests_control.hpp>

namespace server {

class CacheInvalidatorHolder {
 public:
  CacheInvalidatorHolder(components::UpdatingComponentBase& cache,
                         const components::ComponentContext&);

  ~CacheInvalidatorHolder();

 private:
  server::handlers::TestsControl* tests_control_;
  components::UpdatingComponentBase& cache_;
};

}  // namespace server
