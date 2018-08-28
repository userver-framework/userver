#pragma once

#include <atomic>
#include <memory>
#include <mutex>

#include <engine/condition_variable.hpp>
#include <utils/async_event_channel.hpp>

#include "component_config.hpp"
#include "updating_component_base.hpp"

namespace components {

// You need to override UpdatingComponentBase::Update
// then call UpdatingComponentBase::StartPeriodicUpdates after setup
// and UpdatingComponentBase::StopPeriodicUpdates before teardown
template <typename T>
class CachingComponentBase
    : public UpdatingComponentBase,
      public utils::AsyncEventChannel<const std::shared_ptr<T>&> {
 public:
  CachingComponentBase(const ComponentConfig& config, const std::string& name);

  std::shared_ptr<T> Get() const;

 protected:
  void Set(std::shared_ptr<T> value_ptr);
  void Set(T&& value);

  template <typename... Args>
  void Emplace(Args&&... args);

  void Clear();

 private:
  std::shared_ptr<T> cache_;
};

template <typename T>
CachingComponentBase<T>::CachingComponentBase(const ComponentConfig& config,
                                              const std::string& name)
    : UpdatingComponentBase(config, name) {}

template <typename T>
std::shared_ptr<T> CachingComponentBase<T>::Get() const {
  return std::atomic_load(&cache_);
}

template <typename T>
void CachingComponentBase<T>::Set(std::shared_ptr<T> value_ptr) {
  std::atomic_store(&cache_, std::move(value_ptr));
  this->SendEvent(Get());
}

template <typename T>
void CachingComponentBase<T>::Set(T&& value) {
  Emplace(std::move(value));
}

template <typename T>
template <typename... Args>
void CachingComponentBase<T>::Emplace(Args&&... args) {
  Set(std::make_shared<T>(std::forward<Args>(args)...));
}

template <typename T>
void CachingComponentBase<T>::Clear() {
  std::atomic_store(&cache_, {});
}

}  // namespace components
