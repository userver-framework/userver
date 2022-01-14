#pragma once

/// @file userver/taxi_config/storage/component.hpp
/// @brief @copybrief components::TaxiConfig

#include <string_view>
#include <type_traits>

#include <userver/components/loggable_component_base.hpp>
#include <userver/concurrent/async_event_source.hpp>
#include <userver/taxi_config/snapshot.hpp>
#include <userver/taxi_config/source.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace impl {
void CheckRegistered(bool registered);
}  // namespace impl

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
class TaxiConfig final : public LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "taxi-config";

  TaxiConfig(const ComponentConfig&, const ComponentContext&);
  ~TaxiConfig() override;

  /// Use `taxi_config::Source` to get up-to-date config values, or to do
  /// something special on config updates
  taxi_config::Source GetSource();

  template <typename UpdaterComponent>
  class Updater;

  class NoblockSubscriber;

 private:
  static bool RegisterUpdaterName(std::string_view name);
  static TaxiConfig& GetTaxiConfig(const ComponentContext&);

  void OnLoadingCancelled() override;

  void SetConfig(std::string_view updater, const taxi_config::DocsMap& value);
  void NotifyLoadingFailed(std::string_view updater, std::string_view error);

  class Impl;
  utils::FastPimpl<Impl, 704, 8> impl_;
};

/// @brief Class that provides update functionality for the config
template <typename UpdaterComponent>
class TaxiConfig::Updater final {
 public:
  /// Constructor to use in updaters
  explicit Updater(const ComponentContext& context)
      : config_to_update_(GetTaxiConfig(context)) {
    static_assert(std::is_base_of_v<impl::ComponentBase, UpdaterComponent>);
    impl::CheckRegistered(kRegistered);
  }

  /// @brief Set up-to-date config
  void SetConfig(const taxi_config::DocsMap& value_ptr) {
    config_to_update_.SetConfig(UpdaterComponent::kName, value_ptr);
  }

  /// @brief call this method from updater component to notify about errors
  void NotifyLoadingFailed(std::string_view error) {
    config_to_update_.NotifyLoadingFailed(UpdaterComponent::kName, error);
  }

 private:
  TaxiConfig& config_to_update_;

  static inline const bool kRegistered =
      TaxiConfig::RegisterUpdaterName(UpdaterComponent::kName);
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
/// If you use this component, you have to disable loading of other updaters
/// (like TaxiConfigClientUpdater) as there must be only a single component 
/// that sets config values.
///
/// ## Static configuration example:
///
/// @snippet components/minimal_component_list_test.cpp  Sample dynamic config fallback component

// clang-format on
class TaxiConfigFallbacksComponent final : public LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "taxi-config-fallbacks";

  TaxiConfigFallbacksComponent(const ComponentConfig&, const ComponentContext&);

 private:
  TaxiConfig::Updater<TaxiConfigFallbacksComponent> updater_;
};

/// @brief Allows to subscribe to `TaxiConfig` updates without waiting for the
/// first update to complete. Primarily intended for internal use.
class TaxiConfig::NoblockSubscriber final {
 public:
  explicit NoblockSubscriber(TaxiConfig& config_component) noexcept;

  NoblockSubscriber(NoblockSubscriber&&) = delete;
  NoblockSubscriber& operator=(NoblockSubscriber&&) = delete;

  concurrent::AsyncEventSource<const taxi_config::Snapshot&>&
  GetEventSource() noexcept;

 private:
  TaxiConfig& config_component_;
};

}  // namespace components

USERVER_NAMESPACE_END
