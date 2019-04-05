#pragma once

#include <vector>

#include <cache/cache_update_trait.hpp>
#include <components/component_context.hpp>
#include <engine/mutex.hpp>

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

  void RegisterComponentInvalidator(components::ComponentBase& owner,
                                    Callback&& handler);

  void UnregisterComponentInvalidator(components::ComponentBase& owner);

  void InvalidateCaches();

 private:
  template <typename T>
  struct Invalidator {
    T* owner;
    Callback handler;

    Invalidator(T* owner, Callback&& handler)
        : owner(owner), handler(std::move(handler)) {}
  };

  template <typename T>
  void UnregisterInvalidatorGeneric(T& owner,
                                    std::vector<Invalidator<T>>& invalidators);

  engine::Mutex mutex_;
  std::vector<Invalidator<components::CacheUpdateTrait>> cache_invalidators_;
  std::vector<Invalidator<components::ComponentBase>> invalidators_;
};

}  // namespace components
