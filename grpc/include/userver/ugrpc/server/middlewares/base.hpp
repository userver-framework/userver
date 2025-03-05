#pragma once

/// @file userver/ugrpc/server/middlewares/base.hpp
/// @brief @copybrief ugrpc::server::MiddlewareBase

#include <memory>
#include <optional>

#include <google/protobuf/message.h>

#include <userver/components/component_base.hpp>
#include <userver/utils/function_ref.hpp>
#include <userver/utils/impl/internal_tag.hpp>
#include <userver/yaml_config/schema.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <userver/ugrpc/middlewares/runner.hpp>
#include <userver/ugrpc/server/call.hpp>
#include <userver/ugrpc/server/middlewares/fwd.hpp>
#include <userver/ugrpc/server/middlewares/groups.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

/// @brief Context for middleware-specific data during gRPC call.
class MiddlewareCallContext final {
public:
    /// @cond
    MiddlewareCallContext(
        const Middlewares& middlewares,
        CallAnyBase& call,
        utils::function_ref<void()> user_call,
        const dynamic_config::Snapshot& config,
        ::google::protobuf::Message* request
    );
    /// @endcond

    /// @brief Call next plugin, or gRPC handler if none.
    void Next();

    /// @brief Get original gRPC Call
    CallAnyBase& GetCall() const;

    /// @brief Get values extracted from dynamic_config. Snapshot will be
    /// deleted when the last middleware completes.
    const dynamic_config::Snapshot& GetInitialDynamicConfig() const;

private:
    void ClearMiddlewaresResources();

    Middlewares::const_iterator middleware_;
    Middlewares::const_iterator middleware_end_;
    utils::function_ref<void()> user_call_;

    CallAnyBase& call_;

    std::optional<dynamic_config::Snapshot> config_;
    ::google::protobuf::Message* request_;
    bool is_called_from_handle_{false};
};

/// @ingroup userver_base_classes
///
/// @brief Base class for server gRPC middleware
class MiddlewareBase {
public:
    MiddlewareBase();
    MiddlewareBase(const MiddlewareBase&) = delete;
    MiddlewareBase& operator=(const MiddlewareBase&) = delete;
    MiddlewareBase& operator=(MiddlewareBase&&) = delete;

    virtual ~MiddlewareBase();

    /// @brief Handles the gRPC request.
    /// @note You should call context.Next() inside, otherwise the call will be
    /// dropped.
    virtual void Handle(MiddlewareCallContext& context) const = 0;

    /// @brief Request hook. The function is invoked on each request.
    virtual void CallRequestHook(const MiddlewareCallContext& context, google::protobuf::Message& request);

    /// @brief Response hook. The function is invoked on each response.
    virtual void CallResponseHook(const MiddlewareCallContext& context, google::protobuf::Message& response);
};

struct ServiceInfo;

/// @ingroup userver_components userver_base_classes
///
/// @brief Factory that creates specific server middlewares for services.
using MiddlewareFactoryComponentBase = middlewares::MiddlewareFactoryComponentBase<MiddlewareBase, ServiceInfo>;

// clang-format off

/// @ingroup userver_components
///
/// @brief The alias for a short-cut server middleware factory.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// enabled | the flag to enable/disable middleware in the pipeline | true
///
/// ## Example usage:
///
/// @snippet samples/grpc_middleware_service/src/middlewares/server/middleware.hpp gRPC middleware sample - Middleware declaration
///
/// ## Static config example
///
/// @snippet samples/grpc_middleware_service/static_config.yaml gRPC middleware sample - static config grpc-server-middlewares-pipeline

// clang-format on

template <typename Middleware>
using SimpleMiddlewareFactoryComponent =
    middlewares::impl::SimpleMiddlewareFactoryComponent<MiddlewareBase, Middleware, ServiceInfo>;

// clang-format off

/// @ingroup userver_components
///
/// @brief Component to create middlewares pipeline.
///
/// You must register your server middleware in this component.
/// Use `MiddlewareDependencyBuilder` to set a dependency of your middleware from others.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// middlewares | middlewares names and configs to use | `{}`
///
/// ## Static config example
///
/// @snippet grpc/functional_tests/middleware_server/static_config.yaml Sample grpc server middleware pipeline component config

// clang-format on

class MiddlewarePipelineComponent final : public middlewares::impl::AnyMiddlewarePipelineComponent {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of ugrpc::middlewares::MiddlewarePipelineComponent for the server side.
    static constexpr std::string_view kName = "grpc-server-middlewares-pipeline";

    MiddlewarePipelineComponent(const components::ComponentConfig& config, const components::ComponentContext& context);
};

}  // namespace ugrpc::server

template <>
inline constexpr bool components::kHasValidate<ugrpc::server::MiddlewarePipelineComponent> = true;

template <>
inline constexpr auto components::kConfigFileMode<ugrpc::server::MiddlewarePipelineComponent> =
    ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
