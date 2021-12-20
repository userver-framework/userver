#pragma once

/// @file userver/ugrpc/client/client_factory_component.hpp
/// @brief @copybrief ugrpc::client::ClientFactoryComponent

#include <userver/components/loggable_component_base.hpp>

#include <userver/ugrpc/client/client_factory.hpp>
#include <userver/ugrpc/client/queue_holder.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// @ingroup userver_components
///
/// @brief Provides a ClientFactory in the component system
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// task-processor | the task processor for blocking channel creation | -
/// channel-args | a map of channel arguments, see gRPC Core docs | {}
///
/// @see https://grpc.github.io/grpc/core/group__grpc__arg__keys.html
class ClientFactoryComponent final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "grpc-client-factory";

  ClientFactoryComponent(const components::ComponentConfig& config,
                         const components::ComponentContext& context);

  ClientFactory& GetFactory();

 private:
  std::optional<QueueHolder> queue_;
  std::optional<ClientFactory> factory_;
};

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
