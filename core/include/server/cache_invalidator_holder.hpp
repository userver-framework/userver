#pragma once

#include <cache/cache_update_trait.hpp>
#include <cache/testsuite_support.hpp>
#include <cache/update_type.hpp>
#include <components/component_context.hpp>

namespace components {
class TestsuiteSupport;
}  // namespace components

namespace server {

class CacheInvalidatorHolder final {
 public:
  CacheInvalidatorHolder(components::CacheUpdateTrait& cache,
                         components::TestsuiteSupport& testsuite_support);

  ~CacheInvalidatorHolder();

 private:
  components::TestsuiteSupport& testsuite_support_;
  components::CacheUpdateTrait& cache_;
};

template <typename T>
class ComponentInvalidatorHolder final {
 public:
  ComponentInvalidatorHolder(T& component,
                             const components::ComponentContext& context,
                             void (T::*invalidate_method)())
      : testsuite_support_{context
                               .FindComponent<components::TestsuiteSupport>()},
        component_(component) {
    testsuite_support_.RegisterComponentInvalidator(
        component_, [component = &component_, invalidate_method]() {
          (component->*invalidate_method)();
        });
  }

  ~ComponentInvalidatorHolder() {
    testsuite_support_.UnregisterComponentInvalidator(component_);
  }

 private:
  components::TestsuiteSupport& testsuite_support_;
  T& component_;
};

}  // namespace server
