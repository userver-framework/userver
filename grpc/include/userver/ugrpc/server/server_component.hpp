#pragma once

#include <userver/components/loggable_component_base.hpp>

#include <userver/ugrpc/server/server.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

/// @ingroup userver_components
///
/// @brief Component that configures and manages the gRPC server.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// port | the port to use for all gRPC services, or 0 to pick any available | -
class ServerComponent final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "grpc-server";

  ServerComponent(const components::ComponentConfig& config,
                  const components::ComponentContext& context);

  /// @returns The contained Server instance
  /// @note All configuration must be performed at the components loading stage
  Server& GetServer() noexcept;

 private:
  void OnAllComponentsLoaded() override;

  void OnAllComponentsAreStopping() override;

  Server server_;
};

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
