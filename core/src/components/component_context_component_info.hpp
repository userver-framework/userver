#pragma once

#include <atomic>
#include <memory>
#include <set>
#include <string>

#include <userver/components/impl/component_base.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/mutex.hpp>

#include "impl/component_name_from_info.hpp"

USERVER_NAMESPACE_BEGIN

namespace components::impl {

enum class ComponentLifetimeStage {
  kNull,
  kCreated,
  kRunning,
  kReadyForClearing
};

class StageSwitchingCancelledException : public std::runtime_error {
 public:
  explicit StageSwitchingCancelledException(const std::string& message);
};

class ComponentInfo final {
 public:
  explicit ComponentInfo(std::string name);

  ComponentNameFromInfo Name() const { return ComponentNameFromInfo{name_}; }

  void SetComponent(std::unique_ptr<ComponentBase>&& component);
  void ClearComponent();
  ComponentBase* GetComponent() const;
  ComponentBase* WaitAndGetComponent() const;

  void AddItDependsOn(ComponentNameFromInfo component);
  void AddDependsOnIt(ComponentNameFromInfo component);

  bool CheckItDependsOn(ComponentNameFromInfo component) const;
  bool CheckDependsOnIt(ComponentNameFromInfo component) const;

  template <typename Func>
  void ForEachItDependsOn(const Func& func) const {
    std::unique_lock lock{mutex_};
    if (stage_ == ComponentLifetimeStage::kNull) {
      auto it_depends_on = it_depends_on_;
      lock.unlock();

      for (const auto& component : it_depends_on) {
        func(component);
      }
    } else {
      lock.unlock();

      // Only readers remain, could read without copying
      for (const auto& component : it_depends_on_) {
        func(component);
      }
    }
  }

  template <typename Func>
  void ForEachDependsOnIt(const Func& func) const {
    std::unique_lock lock{mutex_};
    if (stage_ == ComponentLifetimeStage::kNull ||
        stage_ == ComponentLifetimeStage::kCreated) {
      auto depends_on_it = depends_on_it_;
      lock.unlock();

      for (const auto& component : depends_on_it) {
        func(component);
      }
    } else {
      lock.unlock();

      // Only readers remain, could read without copying
      for (const auto& component : depends_on_it_) {
        func(component);
      }
    }
  }

  void SetStageSwitchingCancelled(bool cancelled);

  void OnLoadingCancelled();
  void OnAllComponentsLoaded();
  void OnAllComponentsAreStopping();

  void SetStage(ComponentLifetimeStage stage);
  ComponentLifetimeStage GetStage() const;
  void WaitStage(ComponentLifetimeStage stage, std::string method_name) const;

  std::string GetDependencies() const;

 private:
  bool HasComponent() const;
  std::unique_ptr<ComponentBase> ExtractComponent();

  const std::string name_;

  mutable engine::Mutex mutex_;
  mutable engine::ConditionVariable cv_;
  std::unique_ptr<ComponentBase> component_;
  std::set<ComponentNameFromInfo> it_depends_on_;
  std::set<ComponentNameFromInfo> depends_on_it_;
  ComponentLifetimeStage stage_ = ComponentLifetimeStage::kNull;
  bool stage_switching_cancelled_{false};
  std::atomic<bool> on_loading_cancelled_called_{false};
};

}  // namespace components::impl

USERVER_NAMESPACE_END
