#pragma once

#include <atomic>
#include <memory>
#include <mutex>

#include <engine/condition_variable.hpp>

#include "component_config.hpp"
#include "updating_component_base.hpp"

namespace components {

// You need to override UpdatingComponentBase::Update
// then call UpdatingComponentBase::StartPeriodicUpdates after setup
// and UpdatingComponentBase::StopPeriodicUpdates before teardown
template <typename T>
class CachingComponentBase : public UpdatingComponentBase {
 public:
  CachingComponentBase(const ComponentConfig& config, const std::string& name);

  void WaitCacheFill() const;

  std::shared_ptr<T> Get() const;

 protected:
  void Set(std::shared_ptr<T> value_ptr);
  void Set(T&& value);
  void Clear();

 private:
  mutable std::mutex cache_fill_mutex_;
  engine::ConditionVariable cache_fill_cv_;
  std::shared_ptr<T> cache_;
};

template <typename T>
CachingComponentBase<T>::CachingComponentBase(const ComponentConfig& config,
                                              const std::string& name)
    : UpdatingComponentBase(config, name) {}

template <typename T>
void CachingComponentBase<T>::WaitCacheFill() const {
  std::unique_lock<std::mutex> lock(cache_fill_mutex_);
  cache_fill_cv_.Wait(lock, [this] { return !!Get(); });
}

template <typename T>
std::shared_ptr<T> CachingComponentBase<T>::Get() const {
  return std::atomic_load(&cache_);
}

template <typename T>
void CachingComponentBase<T>::Set(std::shared_ptr<T> value_ptr) {
  auto old_cache = std::atomic_exchange(&cache_, std::move(value_ptr));
  if (!old_cache) {
    std::lock_guard<std::mutex> lock(cache_fill_mutex_);
    cache_fill_cv_.NotifyAll();
  }
}

template <typename T>
void CachingComponentBase<T>::Set(T&& value) {
  Set(std::make_shared<T>(std::move(value)));
}

template <typename T>
void CachingComponentBase<T>::Clear() {
  std::atomic_store(&cache_, {});
}

}  // namespace components
