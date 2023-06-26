#pragma once

/// @file userver/ugrpc/server/logging/component.hpp
/// @brief @copybrief ugrpc::server::middlewares::log::Component

#include <optional>

#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

/// Server logging middleware
namespace ugrpc::server::middlewares::log {

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
  static constexpr std::string_view kName = "grpc-server-logging";

  Component(const components::ComponentConfig& config,
            const components::ComponentContext& context);

  std::shared_ptr<MiddlewareBase> GetMiddleware() override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  std::size_t max_size_;
  logging::Level msg_log_level_;
  std::optional<logging::Level> local_log_level_;
};

}  // namespace ugrpc::server::middlewares::log

USERVER_NAMESPACE_END
