#pragma once

/// @file userver/ugrpc/server/service_component_base.hpp
/// @brief @copybrief ugrpc::server::ServiceComponentBase

#include <atomic>

#include <userver/components/component_base.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/yaml_config/fwd.hpp>

#include <userver/ugrpc/middlewares/runner.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/ugrpc/server/middlewares/fwd.hpp>
#include <userver/ugrpc/server/service_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

class ServerComponent;
class GenericServiceBase;

namespace impl {

using MiddlewareRunner = USERVER_NAMESPACE::ugrpc::middlewares::RunnerComponentBase<MiddlewareBase, ServiceInfo>;

}

/// @brief Service info for the middleware
struct ServiceInfo final {
    std::string full_service_name{};
};

/// @ingroup userver_component_names
/// @brief The default name of ugrpc::middlewares::MiddlewarePipelineComponent for the server side
inline constexpr std::string_view kMiddlewarePipelineName = "grpc-server-middlewares-pipeline";

// clang-format off

/// @ingroup userver_components userver_base_classes
///
/// @brief Base class for all the gRPC service components.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// task-processor | the task processor to use for responses | taken from grpc-server.service-defaults
/// disable-user-pipeline-middlewares | flag to disable `groups::User` middlewares from pipeline | false
/// disable-all-pipeline-middlewares | flag to disable all middlewares from pipline | false
/// middlewares | middlewares names to use | `{}` (use server defaults)

// clang-format on

class ServiceComponentBase : public impl::MiddlewareRunner {
public:
    ServiceComponentBase(const components::ComponentConfig& config, const components::ComponentContext& context);

    static yaml_config::Schema GetStaticConfigSchema();

protected:
    /// Derived classes must store the actual service class in a field and call
    /// RegisterService with it
    void RegisterService(ServiceBase& service);

    /// @overload
    void RegisterService(GenericServiceBase& service);

private:
    ServerComponent& server_;
    ServiceConfig config_;
    std::atomic<bool> registered_{false};
    ServiceInfo info_{};
};

namespace impl {

template <typename ServiceInterface>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ServiceComponentBase : public server::ServiceComponentBase, public ServiceInterface {
    static_assert(std::is_base_of_v<ServiceBase, ServiceInterface> || std::is_base_of_v<GenericServiceBase, ServiceInterface>);

public:
    ServiceComponentBase(const components::ComponentConfig& config, const components::ComponentContext& context)
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
