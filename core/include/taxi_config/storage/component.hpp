#pragma once

/// @file taxi_config/storage/component.hpp
/// @brief @copybrief components::TaxiConfig

#include <chrono>
#include <exception>
#include <memory>
#include <optional>
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

class HttpClient;

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
/// fs-cache-path | path to the file to read and dump a config cache; set to empty string to disable reading and dumping configs to FS | -
/// fs-task-processor | name of the task processor to run the blocking file write operations | -
///
/// ## Configuration example:
///
/// @snippet components/common_component_list_test.cpp  Sample taxi config component config

// clang-format on

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class TaxiConfig final : public LoggableComponentBase {
 public:
  static constexpr const char* kName = "taxi-config";

  TaxiConfig(const ComponentConfig&, const ComponentContext&);
  ~TaxiConfig() override;

  /// Get config, may block if no config is available yet
  std::shared_ptr<const taxi_config::Config> Get() const;

  taxi_config::Source GetSource();

  // TODO TAXICOMMON-3830 remove
  template <typename T>
  utils::SharedReadablePtr<T> GetAs() const {
    auto config = Get();
    const T& ptr = config->Get<T>();
    // Use shared_ptr's aliasing constructor
    return std::shared_ptr<const T>{std::move(config), &ptr};
  }

  /// Get config, always returns something without blocking
  /// (either up-to-date config or bootstrap config)
  std::shared_ptr<const taxi_config::BootstrapConfig> GetBootstrap() const;

  /// Get config, never blocks, may return nullptr
  std::shared_ptr<const taxi_config::Config> GetNoblock() const;

  void OnLoadingCancelled() override;

  /// Subscribe to config updates using a member function,
  /// named `OnConfigUpdate` by convention
  template <class Class>
  ::utils::AsyncEventSubscriberScope UpdateAndListen(
      Class* obj, std::string name,
      void (Class::*func)(const std::shared_ptr<const taxi_config::Config>&)) {
    {
      auto value = Get();
      (obj->*func)(value);
    }
    return event_channel_.AddListener(obj, std::move(name), func);
  }

  utils::AsyncEventChannel<const std::shared_ptr<const taxi_config::Config>&>&
  GetEventChannel();

  // TODO TAXICOMMON-3830 remove
  template <class Class>
  ::utils::AsyncEventSubscriberScope UpdateAndListen(Class* obj,
                                                     std::string name,
                                                     void (Class::*func)()) {
    {
      auto value = Get();
      (obj->*func)();
    }
    return event_channel_.AddListener(
        utils::AsyncEventChannelBase::FunctionId{obj}, std::move(name),
        [func, obj](auto&) { (obj->*func)(); });
  }

  class Updater;

 private:
  void SetConfig(std::shared_ptr<const taxi_config::DocsMap> value_ptr);
  void NotifyLoadingFailed(const std::string& updater_error);

  /// non-blocking check if config is available
  bool Has() const;

  bool IsFsCacheEnabled() const;
  void ReadFsCache();
  void WriteFsCache(const taxi_config::DocsMap&);

  void ReadBootstrap(const std::string& bootstrap_fname);

  void DoSetConfig(
      const std::shared_ptr<const taxi_config::DocsMap>& value_ptr);

  // for cache_
  friend taxi_config::impl::Storage& taxi_config::impl::FindStorage(
      const components::ComponentContext& context);
  friend class components::HttpClient;

  utils::AsyncEventChannel<const std::shared_ptr<const taxi_config::Config>&>
      event_channel_;
  std::optional<taxi_config::impl::Storage> cache_;

  // TODO remove in TAXICOMMON-3716
  rcu::Variable<std::shared_ptr<const taxi_config::Config>> cache_ptr_;

  std::shared_ptr<const taxi_config::BootstrapConfig> bootstrap_config_;

  const std::string fs_cache_path_;
  engine::TaskProcessor* fs_task_processor_;
  std::string fs_loading_error_msg_;

  mutable engine::Mutex loaded_mutex_;
  mutable engine::ConditionVariable loaded_cv_;
  bool config_load_cancelled_;

  std::atomic<unsigned> updaters_count_{0};
};

/// @brief Class that provides update functionality for the config
class TaxiConfig::Updater final {
 public:
  explicit Updater(TaxiConfig& config_to_update) noexcept
      : config_to_update_(config_to_update) {
    ++config_to_update_.updaters_count_;
  }

  Updater(Updater&&) = delete;
  Updater& operator=(Updater&&) = delete;

  /// @brief Set up-to-date config
  void SetConfig(std::shared_ptr<const taxi_config::DocsMap> value_ptr) {
    config_to_update_.SetConfig(std::move(value_ptr));
  }

  /// @brief call this method from updater component to notify about errors
  void NotifyLoadingFailed(const std::string& updater_error) {
    config_to_update_.NotifyLoadingFailed(updater_error);
  }

  ~Updater() { --config_to_update_.updaters_count_; }

 private:
  TaxiConfig& config_to_update_;
};

}  // namespace components

namespace taxi_config {

// TODO TAXICOMMON-3830 remove
template <typename Class>
::utils::AsyncEventSubscriberScope UpdateAndListen(
    const components::ComponentContext& context, Class* obj, std::string name,
    void (Class::*func)()) {
  auto& storage = context.FindComponent<components::TaxiConfig>();
  storage.Get();  // wait for the initial config to load
  return storage.UpdateAndListen(obj, std::move(name), func);
}

}  // namespace taxi_config
