#pragma once

/// @file userver/congestion_control/component.hpp
/// @brief @copybrief congestion_control::Component

#include <userver/components/loggable_component_base.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN

namespace congestion_control {

// clang-format off

/// @ingroup userver_components
///
/// @brief Component to limit too active requests, also known as CC.
///
/// ## Dynamic config
/// * @ref USERVER_RPS_CCONTROL
/// * @ref USERVER_RPS_CCONTROL_ENABLED
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// fake-mode | if set, an actual throttling is skipped, but FSM is still working and producing informational logs | false
/// min-cpu | force fake-mode if the current cpu number is less than the specified value | 1
/// only-rtc | if set to true and hostinfo::IsInRtc() returns false then forces the fake-mode | true
/// status-code | HTTP status code for ratelimited responses | 429
///
/// ## Static configuration example:
///
/// @snippet components/common_server_component_list_test.cpp  Sample congestion control component config

// clang-format on

class Component final : public components::LoggableComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of congestion_control::Component component
  static constexpr std::string_view kName = "congestion-control";

  Component(const components::ComponentConfig&,
            const components::ComponentContext&);

  ~Component() override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  void OnConfigUpdate(const dynamic_config::Snapshot& cfg);

  void OnAllComponentsLoaded() override;

  void OnAllComponentsAreStopping() override;

  void ExtendWriter(utils::statistics::Writer& writer);

  struct Impl;
  utils::FastPimpl<Impl, 560, 8> pimpl_;
};

}  // namespace congestion_control

template <>
inline constexpr bool components::kHasValidate<congestion_control::Component> =
    true;

USERVER_NAMESPACE_END
