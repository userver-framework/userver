#pragma once

/// @file userver/ugrpc/server/middlewares/log/component.hpp
/// @brief @copybrief ugrpc::server::middlewares::log::Component

#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/utils/box.hpp>

USERVER_NAMESPACE_BEGIN

/// Server logging middleware
namespace ugrpc::server::middlewares::log {

struct Settings;

// clang-format off

/// @ingroup userver_components userver_base_classes
///
/// @brief Component for gRPC server logging
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// log-level | log level for msg logging | debug
/// msg-size-log-limit | max message size to log, the rest will be truncated | 512

// clang-format on

class Component final : public MiddlewareComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of ugrpc::server::middlewares::log::Component
  static constexpr std::string_view kName = "grpc-server-logging";

  Component(const components::ComponentConfig& config,
            const components::ComponentContext& context);

  ~Component() override;

  std::shared_ptr<MiddlewareBase> GetMiddleware() override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  const utils::Box<Settings> settings_;
};

}  // namespace ugrpc::server::middlewares::log

USERVER_NAMESPACE_END
