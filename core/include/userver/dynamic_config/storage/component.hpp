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
#include <userver/dynamic_config/updates_sink/component.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

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
/// @snippet components/common_component_list_test.cpp  Sample dynamic config component config
///
/// ## Usage example:
/// @snippet components/component_sample_test.cpp  Sample user component runtime config source

// clang-format on
class DynamicConfig final : public DynamicConfigUpdatesSinkBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of components::DynamicConfig
  static constexpr std::string_view kName = "dynamic-config";

  class NoblockSubscriber;

  DynamicConfig(const ComponentConfig&, const ComponentContext&);
  ~DynamicConfig() override;

  /// Use `dynamic_config::Source` to get up-to-date config values, or to do
  /// something special on config updates
  dynamic_config::Source GetSource();

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  void OnLoadingCancelled() override;

  void SetConfig(std::string_view updater,
                 dynamic_config::DocsMap&& value) override;

  void SetConfig(std::string_view updater,
                 const dynamic_config::DocsMap& value) override;

  void NotifyLoadingFailed(std::string_view updater,
                           std::string_view error) override;

  class Impl;
  std::unique_ptr<Impl> impl_;
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
