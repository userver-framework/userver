#pragma once

/// @file userver/dynamic_config/storage/component.hpp
/// @brief @copybrief components::DynamicConfig

#include <memory>
#include <string_view>
#include <type_traits>

#include <userver/components/loggable_component_base.hpp>
#include <userver/concurrent/async_event_source.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/source.hpp>
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
/// @snippet components/common_component_list_test.cpp  Sample dynamic config component config
///
/// ## Usage example:
/// @snippet components/component_sample_test.cpp  Sample user component runtime config source

// clang-format on
class DynamicConfig final : public LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "dynamic-config";

  DynamicConfig(const ComponentConfig&, const ComponentContext&);
  ~DynamicConfig() override;

  /// Use `dynamic_config::Source` to get up-to-date config values, or to do
  /// something special on config updates
  dynamic_config::Source GetSource();

  template <typename UpdaterComponent>
  class Updater;

  class NoblockSubscriber;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  static bool RegisterUpdaterName(std::string_view name);
  static DynamicConfig& GetDynamicConfig(const ComponentContext&);

  void OnLoadingCancelled() override;

  void SetConfig(std::string_view updater,
                 const dynamic_config::DocsMap& value);
  void NotifyLoadingFailed(std::string_view updater, std::string_view error);

  class Impl;
  std::unique_ptr<Impl> impl_;
};

/// @brief Class that provides update functionality for the config
template <typename UpdaterComponent>
class DynamicConfig::Updater final {
 public:
  /// Constructor to use in updaters
  explicit Updater(const ComponentContext& context)
      : config_to_update_(GetDynamicConfig(context)) {
    static_assert(std::is_base_of_v<impl::ComponentBase, UpdaterComponent>);
    impl::CheckRegistered(kRegistered);
  }

  /// @brief Set up-to-date config
  void SetConfig(const dynamic_config::DocsMap& value_ptr) {
    config_to_update_.SetConfig(UpdaterComponent::kName, value_ptr);
  }

  /// @brief call this method from updater component to notify about errors
  void NotifyLoadingFailed(std::string_view error) {
    config_to_update_.NotifyLoadingFailed(UpdaterComponent::kName, error);
  }

 private:
  DynamicConfig& config_to_update_;

  static inline const bool kRegistered =
      DynamicConfig::RegisterUpdaterName(UpdaterComponent::kName);
};

/// @brief Allows to subscribe to `DynamicConfig` updates without waiting for
/// the first update to complete. Primarily intended for internal use.
class DynamicConfig::NoblockSubscriber final {
 public:
  explicit NoblockSubscriber(DynamicConfig& config_component) noexcept;

  NoblockSubscriber(NoblockSubscriber&&) = delete;
  NoblockSubscriber& operator=(NoblockSubscriber&&) = delete;

  concurrent::AsyncEventSource<const dynamic_config::Snapshot&>&
  GetEventSource() noexcept;

 private:
  DynamicConfig& config_component_;
};

template <>
inline constexpr bool kHasValidate<DynamicConfig> = true;

}  // namespace components

USERVER_NAMESPACE_END
