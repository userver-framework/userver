#pragma once

/// @file taxi_config/storage/component.hpp
/// @brief @copybrief components::TaxiConfig

#include <chrono>
#include <exception>
#include <memory>
#include <string>
#include <utility>

#include <cache/cache_update_trait.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/loggable_component_base.hpp>
#include <rcu/rcu.hpp>
#include <taxi_config/config.hpp>
#include <taxi_config/config_ptr.hpp>
#include <utils/async_event_channel.hpp>
#include <utils/shared_readable_ptr.hpp>

namespace components {

// clang-format off

/// @ingroup userver_components
///
/// @brief Component that stores the runtime config.
///
/// The component must be configured in service config.
///
/// ## Available options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// bootstrap-path | path to JSON file with initial runtime options required for the service bootstrap | -
/// fs-cache-path | path to the file to dump a config cache | -
/// fs-task-processor-name | name of the task processor to run the blocking file write operations | -
///
/// ## Configuration example:
///
/// @snippet components/common_component_list_test.cpp  Sample taxi config component config

// clang-format on

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class TaxiConfig : public LoggableComponentBase,
                   public utils::AsyncEventChannel<
                       const std::shared_ptr<const taxi_config::Config>&> {
 public:
  static constexpr const char* kName = "taxi-config";

  TaxiConfig(const ComponentConfig&, const ComponentContext&);
  ~TaxiConfig() override;

  /// Get config, may block if no config is available yet
  std::shared_ptr<const taxi_config::Config> Get() const;

  template <typename T>
  utils::SharedReadablePtr<T> GetAs() const {
    auto config = Get();
    const T& ptr = config->Get<T>();
    // Use shared_ptr's aliasing constructor
    return std::shared_ptr<const T>{std::move(config), &ptr};
  }

  void NotifyLoadingFailed(const std::string& updater_error);

  /// Get config, always returns something without blocking
  /// (either up-to-date config or bootstrap config)
  std::shared_ptr<const taxi_config::BootstrapConfig> GetBootstrap() const;

  /// Get config, never blocks, may return nullptr
  std::shared_ptr<const taxi_config::Config> GetNoblock() const;

  /// Set up-to-date config. Must be used by config updaters only
  /// (e.g. config client).
  void SetConfig(std::shared_ptr<const taxi_config::DocsMap> value_ptr);

  void OnLoadingCancelled() override;

  /// Subscribe to config updates using a member function,
  /// named `OnConfigUpdate` by convention
  template <class Class>
  [[nodiscard]] ::utils::AsyncEventSubscriberScope UpdateAndListen(
      Class* obj, std::string name,
      void (Class::*func)(const std::shared_ptr<const taxi_config::Config>&)) {
    {
      auto value = Get();
      (obj->*func)(value);
    }
    return AddListener(obj, std::move(name), func);
  }

 private:
  /// non-blocking check if config is available
  bool Has() const;

  bool IsFsCacheEnabled() const;
  void ReadFsCache();
  void WriteFsCache(const taxi_config::DocsMap&);

  void ReadBootstrap(const std::string& bootstrap_fname);

  void DoSetConfig(
      const std::shared_ptr<const taxi_config::DocsMap>& value_ptr);

 private:
  // for cache_
  friend const taxi_config::impl::Storage& taxi_config::impl::FindStorage(
      const components::ComponentContext& context);

  std::shared_ptr<const taxi_config::BootstrapConfig> bootstrap_config_;
  bool config_load_cancelled_;

  engine::TaskProcessor& fs_task_processor_;
  const std::string fs_cache_path_;

  mutable engine::ConditionVariable loaded_cv_;
  mutable engine::Mutex loaded_mutex_;
  taxi_config::impl::Storage cache_;
  std::string loading_error_msg_;
};

}  // namespace components

namespace taxi_config {

/// Subscribe to config updates using a member function,
/// named `OnConfigUpdate` by convention
template <typename Class>
[[nodiscard]] ::utils::AsyncEventSubscriberScope UpdateAndListen(
    const components::ComponentContext& context, Class* obj, std::string name,
    void (Class::*func)()) {
  const auto& storage = context.FindComponent<components::TaxiConfig>();
  storage.Get();  // wait for the initial config to load
  (obj->*func)();
  return storage.AddListener(utils::AsyncEventChannelBase::FunctionId{obj},
                             std::move(name),
                             [func, obj](auto&) { (obj->*func)(); });
}

}  // namespace taxi_config
