#pragma once

#include <mutex>
#include <vector>

#include <components/cache_update_trait.hpp>
#include <components/component_context.hpp>

namespace components {

class CacheInvalidator : public components::ComponentBase {
 public:
  static constexpr const char* kName = "cache-invalidator";

  CacheInvalidator(const components::ComponentConfig& component_config,
                   const components::ComponentContext& component_context);

  using Callback = utils::PeriodicTask::Callback;

  void RegisterCacheInvalidator(components::CacheUpdateTrait& owner,
                                Callback&& handler);

  void UnregisterCacheInvalidator(components::CacheUpdateTrait& owner);

  void InvalidateCaches(tracing::Span& span);

 private:
  struct Invalidator {
    components::CacheUpdateTrait* owner;
    Callback handler;

    Invalidator(components::CacheUpdateTrait* owner, Callback&& handler)
        : owner(owner), handler(std::move(handler)) {}
  };

  std::mutex mutex_;
  std::vector<Invalidator> cache_invalidators_;
};

}  // namespace components
