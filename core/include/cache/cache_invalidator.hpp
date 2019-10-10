#pragma once

#include <unordered_map>
#include <vector>

#include <cache/cache_update_trait.hpp>
#include <cache/update_type.hpp>
#include <components/component_context.hpp>
#include <engine/mutex.hpp>

namespace components {

class CacheInvalidator final : public components::impl::ComponentBase {
 public:
  static constexpr const char* kName = "cache-invalidator";

  CacheInvalidator(const components::ComponentConfig& component_config,
                   const components::ComponentContext& component_context);

  using CallbackVoid = utils::PeriodicTask::Callback;
  using CallbackUpdateType = std::function<void(cache::UpdateType)>;

  void RegisterCacheInvalidator(components::CacheUpdateTrait& owner,
                                CallbackUpdateType&& handler);

  void UnregisterCacheInvalidator(components::CacheUpdateTrait& owner);

  void RegisterComponentInvalidator(components::impl::ComponentBase& owner,
                                    CallbackVoid&& handler);

  void UnregisterComponentInvalidator(components::impl::ComponentBase& owner);

  void InvalidateCaches(cache::UpdateType update_type);

  void RegisterPeriodicTask(const std::string& name, utils::PeriodicTask& task);

  void UnregisterPeriodicTask(const std::string& name,
                              utils::PeriodicTask& task);

  bool RunPeriodicTask(const std::string& name);

 private:
  template <typename T, typename Callback>
  struct Invalidator {
    T* owner;
    Callback handler;

    Invalidator(T* owner, Callback&& handler)
        : owner(owner), handler(std::move(handler)) {}
  };

  using ComponentInvalidatorStruct =
      Invalidator<components::impl::ComponentBase, CallbackVoid>;
  using CacheInvalidatorStruct =
      Invalidator<components::CacheUpdateTrait, CallbackUpdateType>;

  template <typename T, typename Callback>
  void UnregisterInvalidatorGeneric(
      T& owner, std::vector<Invalidator<T, Callback>>& invalidators);

  engine::Mutex mutex_;

  std::vector<CacheInvalidatorStruct> cache_invalidators_;
  std::vector<ComponentInvalidatorStruct> invalidators_;

  std::unordered_map<std::string, utils::PeriodicTask&> periodic_tasks_;
};

}  // namespace components
