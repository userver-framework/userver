#pragma once

/// @file userver/ugrpc/server/server_component.hpp
/// @brief @copybrief ugrpc::server::ServerComponent

#include <userver/components/loggable_component_base.hpp>

#include <userver/ugrpc/server/server.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

// clang-format off

/// @ingroup userver_components
///
/// @brief Component that configures and manages the gRPC server.
///
/// ## Static options:
/// The component name for static config is `"grpc-server"`.
///
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// port | the port to use for all gRPC services, or 0 to pick any available | -
/// channel-args | a map of channel arguments, see gRPC Core docs | {}
/// native-log-level | min log level for the native gRPC library | 'error'
/// enable-channelz | initialize service with runtime info about gRPC connections | false
///
/// @see https://grpc.github.io/grpc/core/group__grpc__arg__keys.html

// clang-format on
class ServerComponent final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "grpc-server";

  ServerComponent(const components::ComponentConfig& config,
                  const components::ComponentContext& context);

  /// @returns The contained Server instance
  /// @note All configuration must be performed at the components loading stage
  Server& GetServer() noexcept;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  void OnAllComponentsLoaded() override;

  void OnAllComponentsAreStopping() override;

  Server server_;
};

}  // namespace ugrpc::server

template <>
inline constexpr bool components::kHasValidate<ugrpc::server::ServerComponent> =
    true;

USERVER_NAMESPACE_END
