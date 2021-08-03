#pragma once

/// @file userver/taxi_config/storage/component.hpp
/// @brief @copybrief components::TaxiConfig

#include <chrono>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <userver/cache/cache_update_trait.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/concurrent/async_event_channel.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/taxi_config/snapshot.hpp>
#include <userver/taxi_config/source.hpp>

namespace components {

// clang-format off

/// @ingroup userver_components
///
/// @brief Component that stores the runtime config.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// fs-cache-path | path to the file to read and dump a config cache; set to empty string to disable reading and dumping configs to FS | -
/// fs-task-processor | name of the task processor to run the blocking file write operations | -
///
/// ## Static configuration example:
///
/// @snippet components/common_component_list_test.cpp  Sample taxi config component config
///
/// ## Usage example:
/// @snippet components/component_sample_test.cpp  Sample user component runtime config source

// clang-format on

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class TaxiConfig final : public LoggableComponentBase {
 public:
  static constexpr const char* kName = "taxi-config";

  TaxiConfig(const ComponentConfig&, const ComponentContext&);
  ~TaxiConfig() override;

  /// Use `taxi_config::Source` to get up-to-date config values, or to do
  /// something special on config updates
  taxi_config::Source GetSource();

  class Updater;
  class NoblockSubscriber;

  class FallbacksComponent;

 private:
  void OnLoadingCancelled() override;

  void SetConfig(const taxi_config::DocsMap& value);
  void DoSetConfig(const taxi_config::DocsMap& value);
  void NotifyLoadingFailed(const std::string& updater_error);

  bool Has() const;
  void WaitUntilLoaded();

  bool IsFsCacheEnabled() const;
  void ReadFsCache();
  void WriteFsCache(const taxi_config::DocsMap&);

  taxi_config::impl::StorageData cache_;

  const std::string fs_cache_path_;
  engine::TaskProcessor* fs_task_processor_;
  std::string fs_loading_error_msg_;

  std::atomic<bool> is_loaded_;
  mutable engine::Mutex loaded_mutex_;
  mutable engine::ConditionVariable loaded_cv_;
  bool config_load_cancelled_;

  std::atomic<unsigned> updaters_count_{0};
};

/// @brief Class that provides update functionality for the config
class TaxiConfig::Updater final {
 public:
  explicit Updater(TaxiConfig& config_to_update) noexcept;

  Updater(Updater&&) = delete;
  Updater& operator=(Updater&&) = delete;

  /// @brief Set up-to-date config
  void SetConfig(const taxi_config::DocsMap& value_ptr);

  /// @brief call this method from updater component to notify about errors
  void NotifyLoadingFailed(const std::string& updater_error);

  ~Updater();

 private:
  TaxiConfig& config_to_update_;
};

// clang-format off

/// @ingroup userver_components
///
/// @brief Component that setup runtime configs based on fallbacks from file.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// fallback-path | a path to the fallback config to load the required config names from it | -
///
/// See also the options for components::CachingComponentBase.
///
/// If you use this component, you have to manually disable loading of TaxiConfigClientUpdater as there must be only a single component that sets config values.
///
/// ## Static configuration example:
///
/// @snippet components/common_component_list_test.cpp  Sample taxi config client updater component config

// clang-format on
class TaxiConfig::FallbacksComponent final : public LoggableComponentBase {
 public:
  static constexpr const char* kName = "taxi-config-fallbacks";

  FallbacksComponent(const ComponentConfig&, const ComponentContext&);
};

/// @brief Allows to subscribe to `TaxiConfig` updates without waiting for the
/// first update to complete. Primarily intended for internal use.
class TaxiConfig::NoblockSubscriber final {
 public:
  explicit NoblockSubscriber(TaxiConfig& config_component) noexcept;

  NoblockSubscriber(NoblockSubscriber&&) = delete;
  NoblockSubscriber& operator=(NoblockSubscriber&&) = delete;

  concurrent::AsyncEventChannel<const taxi_config::Snapshot&>&
  GetEventChannel() noexcept;

 private:
  TaxiConfig& config_component_;
};

}  // namespace components
