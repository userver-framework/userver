#pragma once

/// @file userver/ugrpc/server/middlewares/baggage/middleware.hpp
/// @brief @copybrief ugrpc::server::middlewares::baggage::Middleware

#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::baggage {

class Middleware final : public MiddlewareBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of ugrpc::server::middlewares::baggage::Component
    static constexpr std::string_view kName = "grpc-server-baggage";

    /// @brief dependency of this middleware
    static inline const auto kDependency = USERVER_NAMESPACE::middlewares::MiddlewareDependencyBuilder();

    void OnCallStart(MiddlewareCallContext& context) const override;
};

}  // namespace ugrpc::server::middlewares::baggage

USERVER_NAMESPACE_END
