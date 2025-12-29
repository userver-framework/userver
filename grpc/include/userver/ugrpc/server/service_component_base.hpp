#pragma once

/// @file userver/ugrpc/server/service_component_base.hpp
/// @brief @copybrief ugrpc::server::ServiceComponentBase

#include <atomic>

#include <userver/components/component_base.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/middlewares/runner.hpp>
#include <userver/utils/box.hpp>
#include <userver/yaml_config/fwd.hpp>

#include <userver/ugrpc/server/middlewares/fwd.hpp>
#include <userver/ugrpc/server/service_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

class ServerComponent;
class GenericServiceBase;
class MiddlewareBase;
struct ServiceInfo;

namespace impl {

/// @brief The interface for a `ServerComponentBase` component. So, `ServerComponentBase` runs with middlewares.
using MiddlewareRunnerComponentBase = USERVER_NAMESPACE::middlewares::RunnerComponentBase<MiddlewareBase, ServiceInfo>;

}  // namespace impl

/// @ingroup userver_components userver_base_classes
///
/// @brief Base class for all the gRPC service components.
///
/// ## Static options of ugrpc::server::ServiceComponentBase :
/// @include{doc} scripts/docs/en/components_schema/grpc/src/ugrpc/server/service_component_base.md
///
/// Options inherited from @ref middlewares::RunnerComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/middlewares/runner_component_base.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
class ServiceComponentBase : public impl::MiddlewareRunnerComponentBase {
public:
    ServiceComponentBase(const components::ComponentConfig& config, const components::ComponentContext& context);

    ~ServiceComponentBase() override;

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
    utils::Box<ServiceInfo> info_;
};

namespace impl {

template <typename ServiceInterface>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ServiceComponentBase : public server::ServiceComponentBase, public ServiceInterface {
    static_assert(std::is_base_of_v<ServiceBase, ServiceInterface> || std::is_base_of_v<GenericServiceBase, ServiceInterface>);

public:
    ServiceComponentBase(const components::ComponentConfig& config, const components::ComponentContext& context)
        : server::ServiceComponentBase(config, context),
          ServiceInterface()
    {
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
