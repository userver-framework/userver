#pragma once

/// @file userver/ugrpc/server/service_component_base.hpp
/// @brief @copybrief ugrpc::server::ServiceComponentBase

#include <atomic>

#include <userver/components/loggable_component_base.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

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

namespace impl {

template <typename ServiceInterface>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ServiceComponentBase : public server::ServiceComponentBase,
                             public ServiceInterface {
  static_assert(std::is_base_of_v<ServiceBase, ServiceInterface>);

 public:
  ServiceComponentBase(const components::ComponentConfig& config,
                       const components::ComponentContext& context)
      : server::ServiceComponentBase(config, context), ServiceInterface() {
    // At this point the derived class that implements ServiceInterface is not
    // constructed yet. We rely on the implementation detail that the methods of
    // ServiceInterface are never called right after RegisterService. Unless
    // Server starts during the construction of this component (which is an
    // error anyway), we should be fine.
    RegisterService(*this);
  }

 private:
  using server::ServiceComponentBase::RegisterService;
};

}  // namespace impl

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
