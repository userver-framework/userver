#pragma once

/// @file userver/ugrpc/client/middlewares/headers_propagator/middleware.hpp
/// @brief @copybrief ugrpc::client::middlewares::headers_propagator::Middleware

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::headers_propagator {

/// @brief middleware for propagate headers from @ref server::request::GetPropagatedHeaders to the gRPC server.
class Middleware final : public MiddlewareBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of
    /// ugrpc::client::middlewares::headers_propagator::Component.
    static constexpr std::string_view kName = "grpc-client-headers-propagator";

    /// @brief dependency of this middleware. User group.
    static inline const auto kDependency = USERVER_NAMESPACE::middlewares::MiddlewareDependencyBuilder();

    Middleware() = default;

    void PreStartCall(MiddlewareCallContext&) const override;
};

}  // namespace ugrpc::client::middlewares::headers_propagator

USERVER_NAMESPACE_END
