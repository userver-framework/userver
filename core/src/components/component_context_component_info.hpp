#pragma once

#include <atomic>
#include <memory>
#include <set>
#include <string>

#include <components/impl/component_base.hpp>
#include <engine/condition_variable.hpp>
#include <engine/mutex.hpp>

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

  const std::string& Name() const { return name_; }

  void SetComponent(std::unique_ptr<ComponentBase>&& component);
  void ClearComponent();
  ComponentBase* GetComponent() const;
  ComponentBase* WaitAndGetComponent() const;

  void AddItDependsOn(std::string component);
  void AddDependsOnIt(std::string component);

  bool CheckItDependsOn(const std::string& component) const;
  bool CheckDependsOnIt(const std::string& component) const;

  template <typename Func>
  void ForEachItDependsOn(const Func& func) const {
    std::lock_guard<engine::Mutex> lock(mutex_);
    for (const auto& component : it_depends_on_) {
      func(component);
    }
  }

  template <typename Func>
  void ForEachDependsOnIt(const Func& func) const {
    std::lock_guard<engine::Mutex> lock(mutex_);
    for (const auto& component : depends_on_it_) {
      func(component);
    }
  }

  void SetStageSwitchingCancelled(bool cancelled);

  void OnLoadingCancelled();
  void OnAllComponentsLoaded();
  void OnAllComponentsAreStopping();

  void SetStage(ComponentLifetimeStage stage);
  ComponentLifetimeStage GetStage() const;
  void WaitStage(ComponentLifetimeStage stage, std::string method_name) const;

 private:
  bool HasComponent() const;
  std::unique_ptr<ComponentBase> ExtractComponent();

  const std::string name_;

  mutable engine::Mutex mutex_;
  mutable engine::ConditionVariable cv_;
  std::unique_ptr<ComponentBase> component_;
  std::set<std::string> it_depends_on_;
  std::set<std::string> depends_on_it_;
  ComponentLifetimeStage stage_ = ComponentLifetimeStage::kNull;
  bool stage_switching_cancelled_{false};
  std::atomic<bool> on_loading_cancelled_called_{false};
};

}  // namespace components::impl
