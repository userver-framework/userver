#pragma once

#include <atomic>
#include <memory>
#include <mutex>

#include <cache/cache_statistics.hpp>
#include <components/statistics_storage.hpp>
#include <engine/condition_variable.hpp>
#include <server/cache_invalidator_holder.hpp>
#include <taxi_config/config.hpp>
#include <taxi_config/storage/component.hpp>
#include <utils/async_event_channel.hpp>
#include <utils/swappingsmart.hpp>

#include <components/component_base.hpp>
#include <components/component_config.hpp>
#include "cache_config.hpp"
#include "cache_update_trait.hpp"

namespace components {

// You need to override CacheUpdateTrait::Update
// then call CacheUpdateTrait::StartPeriodicUpdates after setup
// and CacheUpdateTrait::StopPeriodicUpdates before teardown
template <typename T>
class CachingComponentBase
    : public LoggableComponentBase,
      public utils::AsyncEventChannel<const std::shared_ptr<T>&>,
      protected CacheUpdateTrait {
 public:
  CachingComponentBase(const ComponentConfig& config, const ComponentContext&,
                       const std::string& name);

  ~CachingComponentBase() override;

  const std::string& Name() const;
  std::shared_ptr<T> Get() const;

 protected:
  void Set(std::shared_ptr<T> value_ptr);
  void Set(T&& value);

  template <typename... Args>
  void Emplace(Args&&... args);

  void Clear();

  formats::json::Value ExtendStatistics(
      const utils::statistics::StatisticsRequest& /*request*/);

  void OnConfigUpdate(const std::shared_ptr<taxi_config::Config>& cfg);

 private:
  utils::statistics::Entry statistics_holder_;
  utils::SwappingSmart<T> cache_;
  utils::AsyncEventSubscriberScope config_subscription_;
  server::CacheInvalidatorHolder cache_invalidator_holder_;
  const std::string name_;
};

template <typename T>
CachingComponentBase<T>::CachingComponentBase(const ComponentConfig& config,
                                              const ComponentContext& context,
                                              const std::string& name)
    : LoggableComponentBase(config, context),
      utils::AsyncEventChannel<const std::shared_ptr<T>&>(name),
      CacheUpdateTrait(CacheConfig(config), name),
      cache_invalidator_holder_(*this, context),
      name_(name) {
  auto& storage =
      context.FindComponent<components::StatisticsStorage>().GetStorage();
  statistics_holder_ = storage.RegisterExtender(
      "cache." + name_, std::bind(&CachingComponentBase<T>::ExtendStatistics,
                                  this, std::placeholders::_1));

  if (config.ParseBool("config-settings", true) &&
      CacheConfigSet::IsConfigEnabled()) {
    auto& taxi_config = context.FindComponent<components::TaxiConfig>();
    OnConfigUpdate(taxi_config.Get());
    config_subscription_ = taxi_config.AddListener(
        this, "cache_" + name, &CachingComponentBase<T>::OnConfigUpdate);
  }
}

template <typename T>
CachingComponentBase<T>::~CachingComponentBase() {
  statistics_holder_.Unregister();
  config_subscription_.Unsubscribe();
}

template <typename T>
const std::string& CachingComponentBase<T>::Name() const {
  return name_;
}

template <typename T>
std::shared_ptr<T> CachingComponentBase<T>::Get() const {
  return cache_.Get();
}

template <typename T>
void CachingComponentBase<T>::Set(std::shared_ptr<T> value_ptr) {
  cache_.Set(value_ptr);
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
  cache_.Clear();
}

template <typename T>
formats::json::Value CachingComponentBase<T>::ExtendStatistics(
    const utils::statistics::StatisticsRequest& /*request*/) {
  /* Make copy to be able to make a more consistent combined statistics */
  const auto full = GetStatistics().full_update;
  const auto incremental = GetStatistics().incremental_update;
  const auto any = cache::CombineStatistics(full, incremental);

  formats::json::ValueBuilder builder;
  builder[cache::kStatisticsNameFull] = cache::StatisticsToJson(full);
  builder[cache::kStatisticsNameIncremental] =
      cache::StatisticsToJson(incremental);
  builder[cache::kStatisticsNameAny] = cache::StatisticsToJson(any);

  builder[cache::kStatisticsNameCurrentDocumentsCount] =
      GetStatistics().documents_current_count.load();

  return builder.ExtractValue();
}

template <typename T>
void CachingComponentBase<T>::OnConfigUpdate(
    const std::shared_ptr<taxi_config::Config>& cfg) {
  SetConfig(cfg->Get<components::CacheConfigSet>().GetConfig(name_));
}

}  // namespace components
