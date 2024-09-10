#pragma once

/// @file userver/ugrpc/client/middlewares/log/component.hpp
/// @brief @copybrief ugrpc::client::middlewares::log::Component

#include <userver/ugrpc/client/middlewares/base.hpp>
#include <userver/utils/box.hpp>

USERVER_NAMESPACE_BEGIN

/// Client logging middleware
namespace ugrpc::client::middlewares::log {

struct Settings;

// clang-format off

/// @ingroup userver_components
///
/// @brief Component for gRPC client logging
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// log-level | log level for msg logging | debug
/// msg-size-log-limit | max message size to log, the rest will be truncated | 512
///
/// ## Static configuration example:
///
/// @snippet grpc/functional_tests/basic_chaos/static_config.yaml Sample grpc client logging middleware component config

// clang-format on

class Component final : public MiddlewareComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of ugrpc::client::middlewares::log::Component
  static constexpr std::string_view kName = "grpc-client-logging";

  Component(const components::ComponentConfig& config,
            const components::ComponentContext& context);

  ~Component() override;

  std::shared_ptr<const MiddlewareFactoryBase> GetMiddlewareFactory() override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  const utils::Box<Settings> settings_;
};

}  // namespace ugrpc::client::middlewares::log

USERVER_NAMESPACE_END
