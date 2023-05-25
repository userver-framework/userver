#pragma once

/// @file userver/ugrpc/client/log_middleware/component.hpp
/// @brief @copybrief ugrpc::client::log_middleware::Component

#include <userver/ugrpc/client/middleware_base.hpp>

USERVER_NAMESPACE_BEGIN

/// Client logging middleware
namespace ugrpc::client::log_middleware {

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

// clang-format on

class Component final : public MiddlewareComponentBase {
 public:
  static constexpr std::string_view kName = "grpc-client-logging";

  Component(const components::ComponentConfig& config,
            const components::ComponentContext& context);

  std::shared_ptr<const MiddlewareFactoryBase> GetMiddlewareFactory() override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  std::size_t max_size_;
  logging::Level log_level_;
};

}  // namespace ugrpc::client::log_middleware

USERVER_NAMESPACE_END
