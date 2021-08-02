#pragma once

/// @file userver/congestion_control/component.hpp
/// @brief @copybrief congestion_control::Component

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/taxi_config/snapshot.hpp>
#include <userver/utils/fast_pimpl.hpp>

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
/// @snippet components/common_server_component_list_test.cpp  Sample handler inspect requests component config

// clang-format on

class Component final : public components::LoggableComponentBase {
 public:
  static constexpr const char* kName = "congestion-control";

  Component(const components::ComponentConfig&,
            const components::ComponentContext&);

  ~Component() override;

 private:
  void OnConfigUpdate(const taxi_config::Snapshot& cfg);

  void OnAllComponentsLoaded() override;

  void OnAllComponentsAreStopping() override;

  formats::json::Value ExtendStatistics(
      const utils::statistics::StatisticsRequest& /*request*/);

  struct Impl;
  utils::FastPimpl<Impl, 552, 8> pimpl_;
};

}  // namespace congestion_control
