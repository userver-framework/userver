#pragma once

/// @file userver/ugrpc/server/middlewares/base.hpp
/// @brief @copybrief ugrpc::server::MiddlewareBase

#include <memory>
#include <optional>

#include <google/protobuf/message.h>

#include <userver/components/component_base.hpp>
#include <userver/utils/function_ref.hpp>

#include <userver/ugrpc/server/call.hpp>
#include <userver/ugrpc/server/middlewares/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

/// @brief Context for middleware-specific data during gRPC call
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

    /// @brief Call next plugin, or gRPC handler if none
    void Next();

    /// @brief Get original gRPC Call
    CallAnyBase& GetCall() const;

    /// @brief Get values extracted from dynamic_config. Snapshot will be
    /// deleted when the last middleware completes
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

    /// @brief Handles the gRPC request
    /// @note You should call context.Next() inside, otherwise the call will be
    /// dropped
    virtual void Handle(MiddlewareCallContext& context) const = 0;

    /// @brief Request hook. The function is invoked on each request
    virtual void CallRequestHook(const MiddlewareCallContext& context, google::protobuf::Message& request);

    /// @brief Response hook. The function is invoked on each response
    virtual void CallResponseHook(const MiddlewareCallContext& context, google::protobuf::Message& response);
};

/// @ingroup userver_base_classes
///
/// @brief Base class for middleware component
class MiddlewareComponentBase : public components::ComponentBase {
    using components::ComponentBase::ComponentBase;

public:
    /// @brief Returns a middleware according to the component's settings
    virtual std::shared_ptr<MiddlewareBase> GetMiddleware() = 0;
};

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
