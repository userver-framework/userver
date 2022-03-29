#pragma once

/// @file userver/ugrpc/server/service_component_base.hpp
/// @brief @copybrief ugrpc::server::ServiceComponentBase

#include <atomic>

#include <userver/components/loggable_component_base.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/yaml_config/schema.hpp>

#include <userver/ugrpc/server/service_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

class Server;

/// @ingroup userver_components userver_base_classes
///
/// @brief Base class for all the gRPC service components.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// task-processor | the task processor to use for responses | -
class ServiceComponentBase : public components::LoggableComponentBase {
 public:
  ServiceComponentBase(const components::ComponentConfig& config,
                       const components::ComponentContext& context);

  static yaml_config::Schema GetStaticConfigSchema();

 protected:
  /// Derived classes must store the actual service class in a field and call
  /// RegisterService with it
  void RegisterService(ServiceBase& service);

 private:
  Server& server_;
  engine::TaskProcessor& service_task_processor_;
  std::atomic<bool> registered_{false};
};

}  // namespace ugrpc::server

template <>
inline constexpr bool
    components::kHasValidate<ugrpc::server::ServiceComponentBase> = true;

USERVER_NAMESPACE_END
