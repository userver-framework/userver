#pragma once

/// @file userver/ugrpc/client/middlewares/baggage/middleware.hpp
/// @brief @copybrief ugrpc::client::middlewares::baggage::Middleware

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::baggage {

/// @brief middleware for gRPC client baggage
class Middleware final : public MiddlewareBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of ugrpc::client::middlewares::baggage::Component.
    static constexpr std::string_view kName = "grpc-client-baggage";

    /// @brief dependency of this middleware. User group as default.
    static inline const auto kDependency = USERVER_NAMESPACE::middlewares::MiddlewareDependencyBuilder();

    void PreStartCall(MiddlewareCallContext& context) const override;
};
///

}  // namespace ugrpc::client::middlewares::baggage

USERVER_NAMESPACE_END
